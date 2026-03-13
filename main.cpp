#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>

#include "win_usb.hpp"

int main(int argc, char* argv[]) {
	QGuiApplication app(argc, argv);

	QQmlApplicationEngine engine;

	WIN_USB& win_usb = WIN_USB::instance();

	bool is_usbpcap_installed = false;
	win_usb.is_usbpcap_upper_filter_installed(is_usbpcap_installed);
	auto upper_filters = win_usb.get_usbpcap_upper_filters();

	QVariantList q_filters;
	for (const auto& filter : upper_filters) {
		q_filters.append(QString::fromStdString(filter));
	}

	engine.rootContext()->setContextProperty("isUsbPcapInstalled", is_usbpcap_installed);
	engine.rootContext()->setContextProperty("usbpcapUpperFilters", q_filters);

	QObject::connect(
		&engine,
		&QQmlApplicationEngine::objectCreationFailed,
		&app,
		[]() { QCoreApplication::exit(-1); },
		Qt::QueuedConnection);
	engine.loadFromModule("Mosfer", "Main");

	return app.exec();
}
