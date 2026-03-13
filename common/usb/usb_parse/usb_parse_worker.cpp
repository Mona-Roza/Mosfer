#include "common/usb/usb_parse/usb_parse_worker.hpp"

#include <iomanip>
#include <sstream>

UsbParseWorker::~UsbParseWorker() {
	stop();
}

bool UsbParseWorker::start(ParsedCallback parsed_callback) {
	if (running_.load()) {
		return true;
	}

	if (!parsed_callback) {
		return false;
	}

	parsed_callback_ = std::move(parsed_callback);
	running_.store(true);
	worker_thread_ = std::thread(&UsbParseWorker::run, this);
	return true;
}

void UsbParseWorker::stop() {
	if (!running_.load()) {
		if (worker_thread_.joinable()) {
			worker_thread_.join();
		}
		return;
	}

	running_.store(false);
	queue_cv_.notify_all();

	if (worker_thread_.joinable()) {
		worker_thread_.join();
	}

	std::lock_guard<std::mutex> lock(queue_mutex_);
	std::queue<std::vector<unsigned char>> empty;
	packet_queue_.swap(empty);
}

void UsbParseWorker::enqueue(std::vector<unsigned char>&& packet) {
	if (!running_.load() || packet.empty()) {
		return;
	}

	{
		std::lock_guard<std::mutex> lock(queue_mutex_);
		packet_queue_.push(std::move(packet));
	}
	queue_cv_.notify_one();
}

bool UsbParseWorker::is_running() const {
	return running_.load();
}

void UsbParseWorker::run() {
	while (running_.load()) {
		std::vector<unsigned char> packet;

		{
			std::unique_lock<std::mutex> lock(queue_mutex_);
			queue_cv_.wait(lock, [this]() { return !running_.load() || !packet_queue_.empty(); });

			if (!running_.load() && packet_queue_.empty()) {
				break;
			}

			packet = std::move(packet_queue_.front());
			packet_queue_.pop();
		}

		ParsedUsbPacket parsed = parse_packet(packet);
		parsed_callback_(parsed);
	}

	running_.store(false);
}

ParsedUsbPacket UsbParseWorker::parse_packet(const std::vector<unsigned char>& packet) const {
	ParsedUsbPacket parsed;
	parsed.packet_size = static_cast<std::uint32_t>(packet.size());
	parsed.preview_hex = to_hex_preview(packet, 24);
	return parsed;
}

std::string UsbParseWorker::to_hex_preview(const std::vector<unsigned char>& packet, std::size_t bytes) {
	std::ostringstream stream;
	stream << std::hex << std::setfill('0');

	const std::size_t preview_size = (packet.size() < bytes) ? packet.size() : bytes;
	for (std::size_t i = 0; i < preview_size; ++i) {
		if (i != 0) {
			stream << ' ';
		}
		stream << std::setw(2) << static_cast<int>(packet[i]);
	}

	return stream.str();
}
