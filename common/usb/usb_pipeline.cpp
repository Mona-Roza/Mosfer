#include "common/usb/usb_pipeline.hpp"

#include <sstream>

namespace {
std::string describe_capture_error(int capture_error, unsigned long win_error) {
	if (capture_error == 3 && win_error == 5) {
		return "Access denied opening USBPcap device. Restart the app as administrator.";
	}

	std::ostringstream oss;
	oss << "Capture error=" << capture_error << " WinErr=" << win_error;
	return oss.str();
}
}  // namespace

#if defined(_WIN32)
#include "win_usb.hpp"
#endif

UsbPipeline::~UsbPipeline() {
	stop();
}

bool UsbPipeline::initialize() {
	usbpcap_installed_ = false;
	usbpcap_upper_filters_.clear();
	active_device_path_.clear();
	parsed_packet_count_.store(0);
	running_.store(false);

#if defined(_WIN32)
	WIN_USB& win_usb = WIN_USB::instance();

	bool installed = false;
	if (win_usb.is_usbpcap_upper_filter_installed(installed) != WIN_USB_SUCCESS) {
		return false;
	}
	usbpcap_installed_ = installed;

	if (win_usb.enumerate_usbpcap_upper_filters() != WIN_USB_SUCCESS) {
		return false;
	}

	const std::list<std::string> filters = win_usb.get_usbpcap_upper_filters();
	usbpcap_upper_filters_.assign(filters.begin(), filters.end());

	if (!usbpcap_upper_filters_.empty()) {
		active_device_path_ = usbpcap_upper_filters_.front();
	}
#endif

	return true;
}

bool UsbPipeline::start() {
#if !defined(_WIN32)
	return false;
#else
	last_capture_error_.clear();

	if (running_.load(std::memory_order_relaxed)) {
		return true;
	}

	if (!usbpcap_installed_ || active_device_path_.empty()) {
		last_capture_error_ = "USBPcap is not ready or no device selected";
		return false;
	}

	if (!parse_worker_.start([this](const ParsedUsbPacket& parsed_packet) {
			parsed_packet_count_.fetch_add(1, std::memory_order_relaxed);

			ParsedPacketCallback callback;
			{
				std::lock_guard<std::mutex> lock(callback_mutex_);
				callback = parsed_packet_callback_;
			}

			if (callback) {
				callback(parsed_packet);
			}
		})) {
		last_capture_error_ = "Failed to start parse worker";
		return false;
	}

	if (!capture_worker_.start(
			active_device_path_,
			[this](std::vector<unsigned char>&& packet) { parse_worker_.enqueue(std::move(packet)); },
			[this](int capture_error, unsigned long win_error) {
				last_capture_error_ = describe_capture_error(capture_error, win_error);
				running_.store(false, std::memory_order_relaxed);
			})) {
		parse_worker_.stop();
		if (last_capture_error_.empty()) {
			last_capture_error_ = "Failed to open/configure USBPcap device";
		}
		return false;
	}

	running_.store(true, std::memory_order_relaxed);

	return true;
#endif
}

void UsbPipeline::stop() {
	capture_worker_.stop();
	parse_worker_.stop();
	running_.store(false, std::memory_order_relaxed);
}

bool UsbPipeline::is_running() const {
	return running_.load(std::memory_order_relaxed);
}

bool UsbPipeline::is_usbpcap_installed() const {
	return usbpcap_installed_;
}

const std::vector<std::string>& UsbPipeline::usbpcap_upper_filters() const {
	return usbpcap_upper_filters_;
}

std::uint64_t UsbPipeline::parsed_packet_count() const {
	return parsed_packet_count_.load(std::memory_order_relaxed);
}

void UsbPipeline::set_parsed_packet_callback(ParsedPacketCallback callback) {
	std::lock_guard<std::mutex> lock(callback_mutex_);
	parsed_packet_callback_ = std::move(callback);
}

bool UsbPipeline::set_active_device_path(const std::string& device_path) {
	if (device_path.empty()) {
		return false;
	}

	for (const auto& filter : usbpcap_upper_filters_) {
		if (filter == device_path) {
			active_device_path_ = device_path;
			return true;
		}
	}

	return false;
}

const std::string& UsbPipeline::active_device_path() const {
	return active_device_path_;
}

const std::string& UsbPipeline::last_capture_error() const {
	return last_capture_error_;
}
