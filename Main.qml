import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

ApplicationWindow {
    width: 640
    height: 480
    visible: true
    title: "Mosfer"

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 16
        spacing: 12

        Text {
            text: isUsbPcapInstalled ? "USBPcap is installed." : "USBPcap is not installed."
            font.pixelSize: 16
        }

        Label {
            text: "USBPcap Upper Filters:"
            font.bold: true
            font.pixelSize: 15
        }
        ListView {
            Layout.fillWidth: true
            Layout.fillHeight: true
            model: usbpcapUpperFilters
            delegate: Rectangle {
                width: parent.width
                height: 40
                color: index % 2 === 0 ? "#f0f0f0" : "#ffffff"

                Text {
                    anchors.centerIn: parent
                    text: modelData
                    font.pixelSize: 14
                }
            }
        }
    }
}
