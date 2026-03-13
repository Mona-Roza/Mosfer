#ifndef MOSFER_USB_UI_CONTROLLER_HPP
#define MOSFER_USB_UI_CONTROLLER_HPP

#include <QObject>
#include <QStringList>

#include "common/usb/usb_pipeline.hpp"

class UsbUiController : public QObject {
	Q_OBJECT
	Q_PROPERTY(bool usbPcapInstalled READ usbPcapInstalled NOTIFY stateChanged)
	Q_PROPERTY(QStringList usbPcapUpperFilters READ usbPcapUpperFilters NOTIFY stateChanged)
	Q_PROPERTY(bool captureRunning READ captureRunning NOTIFY stateChanged)
	Q_PROPERTY(qulonglong parsedPacketCount READ parsedPacketCount NOTIFY stateChanged)
	Q_PROPERTY(QStringList recentPackets READ recentPackets NOTIFY stateChanged)
	Q_PROPERTY(int selectedFilterIndex READ selectedFilterIndex WRITE setSelectedFilterIndex NOTIFY stateChanged)
	Q_PROPERTY(QString captureErrorText READ captureErrorText NOTIFY stateChanged)
	Q_PROPERTY(bool elevationRequired READ elevationRequired NOTIFY stateChanged)
	Q_PROPERTY(QString statusText READ statusText NOTIFY stateChanged)

   public:
	explicit UsbUiController(QObject* parent = nullptr);
	~UsbUiController() override;

	bool usbPcapInstalled() const;
	QStringList usbPcapUpperFilters() const;
	bool captureRunning() const;
	qulonglong parsedPacketCount() const;
	QStringList recentPackets() const;
	int selectedFilterIndex() const;
	void setSelectedFilterIndex(int index);
	QString captureErrorText() const;
	bool elevationRequired() const;
	QString statusText() const;

	Q_INVOKABLE void initialize();
	Q_INVOKABLE bool startCapture();
	Q_INVOKABLE void stopCapture();
	Q_INVOKABLE void refresh();
	Q_INVOKABLE void selectFilter(int index);
	Q_INVOKABLE bool restartAsAdministrator();

   signals:
	void stateChanged();

   private:
	void update_cached_state();
	void append_recent_packet(const ParsedUsbPacket& parsed_packet);

	UsbPipeline pipeline_;
	bool initialized_		= false;
	bool usbpcap_installed_ = false;
	QStringList upper_filters_;
	bool capture_running_			= false;
	qulonglong parsed_packet_count_ = 0;
	QStringList recent_packets_;
	int selected_filter_index_ = -1;
	QString capture_error_text_;
	QString status_text_;
};

#endif	// MOSFER_USB_UI_CONTROLLER_HPP
