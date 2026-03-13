#ifndef __WIN_USB_HPP__
#define __WIN_USB_HPP__

#include <tchar.h>
#include <windows.h>
#include <winioctl.h>

#include <atomic>
#include <list>
#include <mutex>
#include <string>
#include <vector>

typedef enum {
	WIN_USB_SUCCESS				= 0,
	WIN_USB_ERROR_LOAD_LIBRARY	= 1,
	WIN_USB_ERROR_GET_PROC		= 2,
	WIN_USB_ERROR_OPEN_DEVICE	= 3,
	WIN_USB_ERROR_READ_DEVICE	= 4,
	WIN_USB_ERROR_NO_DATA		= 5,
	WIN_USB_ERROR_CONFIG_DEVICE = 6,
} win_usb_err_t;

#pragma pack(push, 1)
struct usbpcap_ioctl_size {
	UINT32 size;
};

struct usbpcap_address_filter {
	UINT32 addresses[4];
	BOOLEAN filter_all;
};
#pragma pack(pop)

#ifndef IOCTL_USBPCAP_SETUP_BUFFER
#define IOCTL_USBPCAP_SETUP_BUFFER CTL_CODE(FILE_DEVICE_UNKNOWN, 0x800, METHOD_BUFFERED, FILE_READ_ACCESS)
#endif

#ifndef IOCTL_USBPCAP_START_FILTERING
#define IOCTL_USBPCAP_START_FILTERING CTL_CODE(FILE_DEVICE_UNKNOWN, 0x801, METHOD_BUFFERED, FILE_READ_ACCESS | FILE_WRITE_ACCESS)
#endif

#ifndef IOCTL_USBPCAP_SET_SNAPLEN_SIZE
#define IOCTL_USBPCAP_SET_SNAPLEN_SIZE CTL_CODE(FILE_DEVICE_UNKNOWN, 0x804, METHOD_BUFFERED, FILE_READ_ACCESS)
#endif

typedef long(NTAPI* nt_query_dir_fn_t)(HANDLE, void*, ULONG, BOOLEAN, BOOLEAN, ULONG*, void*);
typedef long(NTAPI* nt_open_dir_fn_t)(HANDLE*, ACCESS_MASK, void*);
typedef long(NTAPI* nt_close_fn_t)(HANDLE);

struct nt_unicode_str {
	USHORT len;
	USHORT max_len;
	PWSTR buff;
};

struct nt_obj_attrs {
	ULONG len;
	HANDLE root_dir;
	nt_unicode_str name;
	ULONG attr;
	PVOID sec_desc;
	PVOID sec_qos;
};

struct nt_obj_dir_info {
	ULONG next_entry_offset;
	ULONG name_len;
	WCHAR name[1];	// flex arr
};

class WIN_USB {
   public:
	// -------------- Methods --------------- //
	static WIN_USB& instance();
	~WIN_USB();

	nt_query_dir_fn_t nt_query_dir = nullptr;
	nt_open_dir_fn_t nt_open_dir   = nullptr;
	nt_close_fn_t nt_close		   = nullptr;

	win_usb_err_t enumerate_usbpcap_upper_filters();

	std::list<std::string> get_usbpcap_upper_filters();

	win_usb_err_t is_usbpcap_upper_filter_installed(bool& installed);

	win_usb_err_t open_capture_device(const std::string& device_path, HANDLE& capture_handle);
	win_usb_err_t close_capture_device(HANDLE capture_handle);
	win_usb_err_t read_capture_packet(HANDLE capture_handle, std::vector<unsigned char>& packet, DWORD max_bytes = 65536);
	win_usb_err_t initialize_capture_session(HANDLE capture_handle, UINT32 snap_len = 65535, UINT32 buffer_len = 1024 * 1024, bool capture_all = true);
	DWORD get_last_win_error() const;

   private:
	// ------------- Propeerties ------------ //
	std::mutex mtx;						  // Mutex for thread safety
	HMODULE nt_dll = nullptr;			  // We are using native Windows API, so we need to load the ntdll.dll
	std::list<std::string> upper_filter;  // List to store device names
	std::atomic<DWORD> last_win_error_ = 0;

	// -------------- Methods --------------- //

	win_usb_err_t load_ntdll();

	win_usb_err_t unload_ntdll();

	WIN_USB();	// Private constructor for singleton pattern
	WIN_USB(const WIN_USB&)			   = delete;
	WIN_USB& operator=(const WIN_USB&) = delete;
	WIN_USB(WIN_USB&&)				   = delete;
	WIN_USB& operator=(WIN_USB&&)	   = delete;
};

#endif	// __WIN_USB_HPP__