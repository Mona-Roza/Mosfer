#include "win_usb.hpp"

#include <stdexcept>

#define FILTER_BUFF_SIZE 4096

#ifndef DIRECTORY_QUERY
#define DIRECTORY_QUERY 0x0001
#endif

// ============== PRIVATE METHODS ============== //
win_usb_err_t WIN_USB::load_ntdll() {
	// Ensure thread safety when loading the library
	std::lock_guard<std::mutex> lock(this->mtx);

	if (nt_dll) return WIN_USB_SUCCESS;	 // Already loaded

	nt_dll = LoadLibraryW(L"ntdll.dll");
	if (!nt_dll) {
		last_win_error_.store(GetLastError());
		return WIN_USB_ERROR_LOAD_LIBRARY;
	}

	nt_query_dir = reinterpret_cast<nt_query_dir_fn_t>(GetProcAddress(nt_dll, "NtQueryDirectoryObject"));
	nt_open_dir	 = reinterpret_cast<nt_open_dir_fn_t>(GetProcAddress(nt_dll, "NtOpenDirectoryObject"));
	nt_close	 = reinterpret_cast<nt_close_fn_t>(GetProcAddress(nt_dll, "NtClose"));

	return WIN_USB_SUCCESS;
}

win_usb_err_t WIN_USB::unload_ntdll() {
	std::lock_guard<std::mutex> lock(this->mtx);  // Ensure thread safety when unloading the library
	if (nt_dll) {
		FreeLibrary(nt_dll);
		nt_dll		 = nullptr;
		nt_query_dir = nullptr;
		nt_open_dir	 = nullptr;
		nt_close	 = nullptr;
	}

	return WIN_USB_SUCCESS;
}

// ============== PUBLIC METHODS ============== //
WIN_USB& WIN_USB::instance() {
	static WIN_USB instance;  // Guaranteed to be destroyed and instantiated on first use
	return instance;
}

WIN_USB::WIN_USB() {
	// Load the ntdll.dll library
	if (load_ntdll() != WIN_USB_SUCCESS) {
		throw std::runtime_error("Failed to load ntdll.dll");
	}

	enumerate_usbpcap_upper_filters();
}

WIN_USB::~WIN_USB() {
	unload_ntdll();
}

win_usb_err_t WIN_USB::enumerate_usbpcap_upper_filters() {
	std::lock_guard<std::mutex> lock(this->mtx);  // Thread safe

	upper_filter.clear();

	DWORD buf_chars = 4096;
	std::vector<wchar_t> dev_buf(buf_chars, L'\0');

	for (;;) {
		DWORD rc = QueryDosDeviceW(nullptr, dev_buf.data(), buf_chars);
		if (rc != 0) {
			break;
		}

		if (GetLastError() != ERROR_INSUFFICIENT_BUFFER) {
			return WIN_USB_ERROR_GET_PROC;
		}

		if (buf_chars > (1u << 20)) {
			return WIN_USB_ERROR_GET_PROC;
		}

		buf_chars *= 2;
		dev_buf.assign(buf_chars, L'\0');
	}

	constexpr const wchar_t* prefix = L"USBPcap";
	constexpr size_t prefix_len		= 7;

	for (const wchar_t* name = dev_buf.data(); *name != L'\0'; name += wcslen(name) + 1) {
		if (_wcsnicmp(name, prefix, prefix_len) != 0) {
			continue;
		}

		std::wstring device_path = L"\\\\.\\";
		device_path += name;

		std::string device_ascii;
		device_ascii.reserve(device_path.size());
		for (wchar_t ch : device_path) {
			if (ch > 0x7F) {
				device_ascii.clear();
				break;
			}
			device_ascii.push_back(static_cast<char>(ch));
		}

		if (!device_ascii.empty()) {
			upper_filter.emplace_back(std::move(device_ascii));
		}
	}

	return WIN_USB_SUCCESS;
}

std::list<std::string> WIN_USB::get_usbpcap_upper_filters() {
	std::lock_guard<std::mutex> lock(this->mtx);  // Thread safe
	return upper_filter;
}

win_usb_err_t WIN_USB::open_capture_device(const std::string& device_path, HANDLE& capture_handle) {
	std::lock_guard<std::mutex> lock(mtx);

	capture_handle = CreateFileA(
		device_path.c_str(),
		GENERIC_READ | GENERIC_WRITE,
		0,
		nullptr,
		OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL,
		nullptr);

	if (capture_handle == INVALID_HANDLE_VALUE) {
		last_win_error_.store(GetLastError());
		return WIN_USB_ERROR_OPEN_DEVICE;
	}

	win_usb_err_t init_rc = initialize_capture_session(capture_handle);
	if (init_rc != WIN_USB_SUCCESS) {
		last_win_error_.store(GetLastError());
		CloseHandle(capture_handle);
		capture_handle = INVALID_HANDLE_VALUE;
		return init_rc;
	}

	last_win_error_.store(0);

	return WIN_USB_SUCCESS;
}

win_usb_err_t WIN_USB::initialize_capture_session(HANDLE capture_handle, UINT32 snap_len, UINT32 buffer_len, bool capture_all) {
	if (capture_handle == nullptr || capture_handle == INVALID_HANDLE_VALUE) {
		last_win_error_.store(ERROR_INVALID_HANDLE);
		return WIN_USB_ERROR_CONFIG_DEVICE;
	}

	DWORD bytes_ret = 0;
	usbpcap_ioctl_size size_data{};
	size_data.size = snap_len;

	if (!DeviceIoControl(capture_handle,
						 IOCTL_USBPCAP_SET_SNAPLEN_SIZE,
						 &size_data,
						 sizeof(size_data),
						 nullptr,
						 0,
						 &bytes_ret,
						 nullptr)) {
		last_win_error_.store(GetLastError());
		return WIN_USB_ERROR_CONFIG_DEVICE;
	}

	size_data.size = buffer_len;
	if (!DeviceIoControl(capture_handle,
						 IOCTL_USBPCAP_SETUP_BUFFER,
						 &size_data,
						 sizeof(size_data),
						 nullptr,
						 0,
						 &bytes_ret,
						 nullptr)) {
		last_win_error_.store(GetLastError());
		return WIN_USB_ERROR_CONFIG_DEVICE;
	}

	usbpcap_address_filter filter{};
	filter.filter_all = capture_all ? TRUE : FALSE;

	if (!DeviceIoControl(capture_handle,
						 IOCTL_USBPCAP_START_FILTERING,
						 &filter,
						 sizeof(filter),
						 nullptr,
						 0,
						 &bytes_ret,
						 nullptr)) {
		last_win_error_.store(GetLastError());
		return WIN_USB_ERROR_CONFIG_DEVICE;
	}

	last_win_error_.store(0);

	return WIN_USB_SUCCESS;
}

win_usb_err_t WIN_USB::close_capture_device(HANDLE capture_handle) {
	if (capture_handle == nullptr || capture_handle == INVALID_HANDLE_VALUE) {
		return WIN_USB_SUCCESS;
	}

	CloseHandle(capture_handle);
	return WIN_USB_SUCCESS;
}

win_usb_err_t WIN_USB::read_capture_packet(HANDLE capture_handle, std::vector<unsigned char>& packet, DWORD max_bytes) {
	if (capture_handle == nullptr || capture_handle == INVALID_HANDLE_VALUE || max_bytes == 0) {
		packet.clear();
		last_win_error_.store(ERROR_INVALID_PARAMETER);
		return WIN_USB_ERROR_READ_DEVICE;
	}

	packet.assign(max_bytes, 0);

	DWORD bytes_read = 0;
	BOOL ok			 = ReadFile(capture_handle, packet.data(), max_bytes, &bytes_read, nullptr);
	if (!ok) {
		const DWORD err = GetLastError();
		packet.clear();
		last_win_error_.store(err);
		if (err == ERROR_NO_DATA) {
			return WIN_USB_ERROR_NO_DATA;
		}
		return WIN_USB_ERROR_READ_DEVICE;
	}

	if (bytes_read == 0) {
		packet.clear();
		last_win_error_.store(ERROR_NO_DATA);
		return WIN_USB_ERROR_NO_DATA;
	}

	packet.resize(bytes_read);
	last_win_error_.store(0);
	return WIN_USB_SUCCESS;
}

DWORD WIN_USB::get_last_win_error() const {
	return last_win_error_.load();
}

win_usb_err_t WIN_USB::is_usbpcap_upper_filter_installed(bool& installed) {
	std::lock_guard<std::mutex> lock(mtx);

	constexpr const wchar_t* USB_CLASS_KEY	   = L"System\\CurrentControlSet\\Control\\Class\\{36FC9E60-C465-11CF-8056-444553540000}";
	constexpr const wchar_t* UPPER_FILTER_NAME = L"USBPcap";
	constexpr size_t FILTER_NAME_LEN		   = 7;	 // USBPcap uzunluğu

	HKEY hkey	 = nullptr;
	DWORD length = 0, type = 0;

	LONG rc = RegOpenKeyEx(HKEY_LOCAL_MACHINE, USB_CLASS_KEY, 0, KEY_QUERY_VALUE, &hkey);
	if (rc != ERROR_SUCCESS) {
		installed = false;
		return WIN_USB_ERROR_GET_PROC;
	}

	rc = RegQueryValueEx(hkey, L"UpperFilters", nullptr, &type, nullptr, &length);
	if (rc != ERROR_SUCCESS || type != REG_MULTI_SZ || length == 0) {
		RegCloseKey(hkey);
		installed = false;
		return WIN_USB_SUCCESS;
	}

	std::vector<wchar_t> multisz(length / sizeof(wchar_t));
	rc = RegQueryValueEx(hkey, L"UpperFilters", nullptr, nullptr, reinterpret_cast<LPBYTE>(multisz.data()), &length);
	RegCloseKey(hkey);

	if (rc != ERROR_SUCCESS) {
		installed = false;
		return WIN_USB_ERROR_GET_PROC;
	}

	// MULTI_SZ içinde filtreyi ara
	installed = false;
	for (size_t i = 0; i < multisz.size(); ++i) {
		if (wcsncmp(&multisz[i], UPPER_FILTER_NAME, FILTER_NAME_LEN) == 0) {
			installed = true;
			break;
		}
		while (i < multisz.size() && multisz[i] != 0) i++;	// bir sonraki stringe geç
	}

	return WIN_USB_SUCCESS;
}
