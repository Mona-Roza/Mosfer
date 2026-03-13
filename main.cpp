#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QTimer>

#include "common/usb/usb_ui_controller.hpp"

int main(int argc, char* argv[]) {
	QGuiApplication app(argc, argv);

	QQmlApplicationEngine engine;
	UsbUiController usb_controller;
	usb_controller.initialize();
	engine.rootContext()->setContextProperty("usbController", &usb_controller);

	QTimer poll_timer;
	poll_timer.setInterval(200);
	QObject::connect(&poll_timer, &QTimer::timeout, &usb_controller, &UsbUiController::refresh);
	poll_timer.start();

	QObject::connect(&app, &QCoreApplication::aboutToQuit, &usb_controller, &UsbUiController::stopCapture);

	QObject::connect(
		&engine,
		&QQmlApplicationEngine::objectCreationFailed,
		&app,
		[]() { QCoreApplication::exit(-1); },
		Qt::QueuedConnection);
	engine.loadFromModule("Mosfer", "Main");

	return app.exec();
}
