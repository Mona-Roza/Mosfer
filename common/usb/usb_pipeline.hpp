#ifndef MOSFER_USB_PIPELINE_HPP
#define MOSFER_USB_PIPELINE_HPP

#include <atomic>
#include <functional>
#include <mutex>
#include <string>
#include <vector>

#include "common/usb/usb_capture/usb_capture_worker.hpp"
#include "common/usb/usb_parse/usb_parse_worker.hpp"

class UsbPipeline {
   public:
	using ParsedPacketCallback = std::function<void(const ParsedUsbPacket&)>;

	UsbPipeline() = default;
	~UsbPipeline();

	bool initialize();
	bool start();
	void stop();
	bool is_running() const;

	bool is_usbpcap_installed() const;
	const std::vector<std::string>& usbpcap_upper_filters() const;
	std::uint64_t parsed_packet_count() const;
	void set_parsed_packet_callback(ParsedPacketCallback callback);
	bool set_active_device_path(const std::string& device_path);
	const std::string& active_device_path() const;
	const std::string& last_capture_error() const;

   private:
	bool usbpcap_installed_ = false;
	std::vector<std::string> usbpcap_upper_filters_;
	std::string active_device_path_;
	std::atomic<std::uint64_t> parsed_packet_count_ = 0;
	std::atomic<bool> running_						= false;
	std::string last_capture_error_;
	mutable std::mutex callback_mutex_;
	ParsedPacketCallback parsed_packet_callback_;

	UsbCaptureWorker capture_worker_;
	UsbParseWorker parse_worker_;
};

#endif	// MOSFER_USB_PIPELINE_HPP
