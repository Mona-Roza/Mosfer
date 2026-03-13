#include "common/usb/usb_capture/usb_capture_worker.hpp"

#include <chrono>

#if defined(_WIN32)
#include "win_usb.hpp"
#endif

UsbCaptureWorker::~UsbCaptureWorker() {
	stop();
}

bool UsbCaptureWorker::start(const std::string& device_path, PacketCallback packet_callback, ErrorCallback error_callback) {
	if (running_.load()) {
		return true;
	}

	if (device_path.empty() || !packet_callback) {
		return false;
	}

	packet_callback_ = std::move(packet_callback);
	error_callback_	 = std::move(error_callback);

#if defined(_WIN32)
	WIN_USB& win_usb = WIN_USB::instance();
	if (win_usb.open_capture_device(device_path, capture_handle_) != WIN_USB_SUCCESS) {
		if (error_callback_) {
			error_callback_(WIN_USB_ERROR_OPEN_DEVICE, win_usb.get_last_win_error());
		}
		capture_handle_ = INVALID_HANDLE_VALUE;
		return false;
	}
#else
	(void)device_path;
#endif

	running_.store(true);
	worker_thread_ = std::thread(&UsbCaptureWorker::run, this);
	return true;
}

void UsbCaptureWorker::stop() {
	if (!running_.load()) {
		if (worker_thread_.joinable()) {
			worker_thread_.join();
		}
		return;
	}

	running_.store(false);
	if (worker_thread_.joinable()) {
		worker_thread_.join();
	}

#if defined(_WIN32)
	if (capture_handle_ != INVALID_HANDLE_VALUE) {
		WIN_USB::instance().close_capture_device(capture_handle_);
		capture_handle_ = INVALID_HANDLE_VALUE;
	}
#endif
}

bool UsbCaptureWorker::is_running() const {
	return running_.load();
}

void UsbCaptureWorker::run() {
#if defined(_WIN32)
	WIN_USB& win_usb = WIN_USB::instance();

	if (capture_handle_ == INVALID_HANDLE_VALUE) {
		running_.store(false);
		return;
	}

	while (running_.load()) {
		std::vector<unsigned char> packet;
		win_usb_err_t rc = win_usb.read_capture_packet(capture_handle_, packet);
		if (rc == WIN_USB_SUCCESS && !packet.empty()) {
			packet_callback_(std::move(packet));
			continue;
		}

		if (rc == WIN_USB_ERROR_READ_DEVICE) {
			if (error_callback_) {
				error_callback_(static_cast<int>(rc), win_usb.get_last_win_error());
			}
			running_.store(false);
			break;
		}

		std::this_thread::sleep_for(std::chrono::milliseconds(2));
	}

#else
	while (running_.load()) {
		std::this_thread::sleep_for(std::chrono::milliseconds(50));
	}
#endif

	running_.store(false);
}
