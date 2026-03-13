#ifndef MOSFER_USB_PARSE_WORKER_HPP
#define MOSFER_USB_PARSE_WORKER_HPP

#include <atomic>
#include <condition_variable>
#include <cstdint>
#include <functional>
#include <mutex>
#include <queue>
#include <string>
#include <thread>
#include <vector>

struct ParsedUsbPacket {
	std::uint32_t packet_size = 0;
	std::string preview_hex;
};

class UsbParseWorker {
   public:
	using ParsedCallback = std::function<void(const ParsedUsbPacket&)>;

	UsbParseWorker() = default;
	~UsbParseWorker();

	bool start(ParsedCallback parsed_callback);
	void stop();
	void enqueue(std::vector<unsigned char>&& packet);
	bool is_running() const;

   private:
	void run();
	ParsedUsbPacket parse_packet(const std::vector<unsigned char>& packet) const;
	static std::string to_hex_preview(const std::vector<unsigned char>& packet, std::size_t bytes);

	std::atomic<bool> running_ = false;
	std::thread worker_thread_;
	ParsedCallback parsed_callback_;

	mutable std::mutex queue_mutex_;
	std::condition_variable queue_cv_;
	std::queue<std::vector<unsigned char>> packet_queue_;
};

#endif	// MOSFER_USB_PARSE_WORKER_HPP
