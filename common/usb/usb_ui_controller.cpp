#include "common/usb/usb_ui_controller.hpp"

#include <QByteArray>
#include <QCoreApplication>
#include <QMetaObject>
#include <QString>

#if defined(_WIN32)
#include <shellapi.h>
#include <windows.h>
#endif

namespace {
std::string qstring_to_utf8_std_string(const QString& value) {
	const QByteArray utf8 = value.toUtf8();
	return std::string(utf8.constData(), static_cast<std::size_t>(utf8.size()));
}
}  // namespace

UsbUiController::UsbUiController(QObject* parent) : QObject(parent) {
	status_text_ = QStringLiteral("Idle");
	pipeline_.set_parsed_packet_callback([this](const ParsedUsbPacket& parsed_packet) {
		QMetaObject::invokeMethod(
			this,
			[this, parsed_packet]() { append_recent_packet(parsed_packet); },
			Qt::QueuedConnection);
	});
}

UsbUiController::~UsbUiController() {
	pipeline_.stop();
}

bool UsbUiController::usbPcapInstalled() const {
	return usbpcap_installed_;
}

QStringList UsbUiController::usbPcapUpperFilters() const {
	return upper_filters_;
}

bool UsbUiController::captureRunning() const {
	return capture_running_;
}

qulonglong UsbUiController::parsedPacketCount() const {
	return parsed_packet_count_;
}

QStringList UsbUiController::recentPackets() const {
	return recent_packets_;
}

int UsbUiController::selectedFilterIndex() const {
	return selected_filter_index_;
}

void UsbUiController::setSelectedFilterIndex(int index) {
	selectFilter(index);
}

QString UsbUiController::captureErrorText() const {
	return capture_error_text_;
}

bool UsbUiController::elevationRequired() const {
	return capture_error_text_.contains(QStringLiteral("administrator"), Qt::CaseInsensitive);
}

QString UsbUiController::statusText() const {
	return status_text_;
}

void UsbUiController::initialize() {
	pipeline_.stop();
	initialized_ = pipeline_.initialize();
	recent_packets_.clear();
	capture_error_text_.clear();
	update_cached_state();
	selected_filter_index_ = upper_filters_.isEmpty() ? -1 : 0;
	if (selected_filter_index_ >= 0) {
		pipeline_.set_active_device_path(qstring_to_utf8_std_string(upper_filters_.at(selected_filter_index_)));
	}

	if (!initialized_) {
		status_text_ = QStringLiteral("Initialization failed");
	} else if (!usbpcap_installed_) {
		status_text_ = QStringLiteral("USBPcap not installed");
	} else if (upper_filters_.isEmpty()) {
		status_text_ = QStringLiteral("No USBPcap devices found");
	} else {
		status_text_ = QStringLiteral("Ready to capture");
	}

	emit stateChanged();
}

bool UsbUiController::startCapture() {
	if (!initialized_) {
		initialize();
	}

	if (!initialized_) {
		status_text_ = QStringLiteral("Initialization failed");
		emit stateChanged();
		return false;
	}

	const bool started = pipeline_.start();
	update_cached_state();
	capture_error_text_ = QString::fromStdString(pipeline_.last_capture_error());
	if (started) {
		status_text_ = QStringLiteral("Capturing on %1")
						   .arg(QString::fromStdString(pipeline_.active_device_path()));
	} else {
		status_text_ = capture_error_text_.isEmpty() ? QStringLiteral("Failed to start capture") : capture_error_text_;
	}
	emit stateChanged();
	return started;
}

void UsbUiController::stopCapture() {
	pipeline_.stop();
	update_cached_state();
	capture_error_text_.clear();
	status_text_ = QStringLiteral("Stopped");
	emit stateChanged();
}

void UsbUiController::refresh() {
	const qulonglong previous_count = parsed_packet_count_;
	const bool previous_running		= capture_running_;
	const QString previous_error	= capture_error_text_;

	update_cached_state();
	capture_error_text_ = QString::fromStdString(pipeline_.last_capture_error());
	if (parsed_packet_count_ != previous_count || capture_running_ != previous_running) {
		emit stateChanged();
		return;
	}

	if (capture_error_text_ != previous_error) {
		if (!capture_error_text_.isEmpty()) {
			status_text_ = capture_error_text_;
		}
		emit stateChanged();
	}
}

void UsbUiController::selectFilter(int index) {
	if (index < 0 || index >= upper_filters_.size()) {
		return;
	}

	if (capture_running_) {
		stopCapture();
	}

	selected_filter_index_ = index;
	const bool selected	   = pipeline_.set_active_device_path(qstring_to_utf8_std_string(upper_filters_.at(index)));
	if (selected) {
		status_text_ = QStringLiteral("Selected %1").arg(upper_filters_.at(index));
	} else {
		status_text_ = QStringLiteral("Failed to select device");
	}

	emit stateChanged();
}

bool UsbUiController::restartAsAdministrator() {
#if !defined(_WIN32)
	return false;
#else
	const QString program = QCoreApplication::applicationFilePath();
	QStringList arguments = QCoreApplication::arguments();
	if (!arguments.isEmpty()) {
		arguments.removeFirst();
	}

	QStringList quoted_arguments;
	for (const QString& argument : arguments) {
		QString escaped = argument;
		escaped.replace('"', QStringLiteral("\\\""));
		quoted_arguments.append(QStringLiteral("\"") + escaped + QStringLiteral("\""));
	}

	const QString parameters = quoted_arguments.join(' ');
	HINSTANCE result		 = ShellExecuteW(
		nullptr,
		L"runas",
		reinterpret_cast<LPCWSTR>(program.utf16()),
		parameters.isEmpty() ? nullptr : reinterpret_cast<LPCWSTR>(parameters.utf16()),
		nullptr,
		SW_SHOWNORMAL);

	const auto result_code = reinterpret_cast<INT_PTR>(result);
	if (result_code <= 32) {
		status_text_ = QStringLiteral("Failed to request administrator restart");
		emit stateChanged();
		return false;
	}

	QCoreApplication::quit();
	return true;
#endif
}

void UsbUiController::update_cached_state() {
	usbpcap_installed_	 = pipeline_.is_usbpcap_installed();
	capture_running_	 = pipeline_.is_running();
	parsed_packet_count_ = static_cast<qulonglong>(pipeline_.parsed_packet_count());

	upper_filters_.clear();
	for (const auto& filter : pipeline_.usbpcap_upper_filters()) {
		upper_filters_.append(QString::fromStdString(filter));
	}
}

void UsbUiController::append_recent_packet(const ParsedUsbPacket& parsed_packet) {
	const QString line = QStringLiteral("%1 B  |  %2")
							 .arg(static_cast<qulonglong>(parsed_packet.packet_size), 0, 10)
							 .arg(QString::fromStdString(parsed_packet.preview_hex));

	recent_packets_.prepend(line);
	while (recent_packets_.size() > 120) {
		recent_packets_.removeLast();
	}

	emit stateChanged();
}
