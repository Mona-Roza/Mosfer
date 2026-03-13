#ifndef MOSFER_USB_CAPTURE_WORKER_HPP
#define MOSFER_USB_CAPTURE_WORKER_HPP

#include <atomic>
#include <functional>
#include <string>
#include <thread>
#include <vector>

#if defined(_WIN32)
#include <windows.h>
#endif

class UsbCaptureWorker {
   public:
	using PacketCallback = std::function<void(std::vector<unsigned char>&&)>;
	using ErrorCallback	 = std::function<void(int, unsigned long)>;

	UsbCaptureWorker() = default;
	~UsbCaptureWorker();

	bool start(const std::string& device_path, PacketCallback packet_callback, ErrorCallback error_callback = nullptr);
	void stop();
	bool is_running() const;

   private:
	void run();

	std::atomic<bool> running_ = false;
	std::thread worker_thread_;
	PacketCallback packet_callback_;
	ErrorCallback error_callback_;

#if defined(_WIN32)
	HANDLE capture_handle_ = INVALID_HANDLE_VALUE;
#endif
};

#endif	// MOSFER_USB_CAPTURE_WORKER_HPP
