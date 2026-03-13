#ifndef __WIN_USB_HPP__
#define __WIN_USB_HPP__

#include <tchar.h>
#include <windows.h>

#include <list>
#include <mutex>
#include <string>
#include <vector>

typedef enum {
	WIN_USB_SUCCESS			   = 0,
	WIN_USB_ERROR_LOAD_LIBRARY = 1,
	WIN_USB_ERROR_GET_PROC	   = 2,
} win_usb_err_t;

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

   private:
	// ------------- Propeerties ------------ //
	std::mutex mtx;						  // Mutex for thread safety
	HMODULE nt_dll = nullptr;			  // We are using native Windows API, so we need to load the ntdll.dll
	std::list<std::string> upper_filter;  // List to store device names

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