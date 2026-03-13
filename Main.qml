import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

ApplicationWindow {
    width: 980
    height: 620
    visible: true
    title: "Mosfer USB Monitor"
    color: "#0b1220"

    Rectangle {
        anchors.fill: parent
        gradient: Gradient {
            GradientStop {
                position: 0.0
                color: "#0b1220"
            }
            GradientStop {
                position: 1.0
                color: "#111d32"
            }
        }
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 20
        spacing: 14

        Rectangle {
            Layout.fillWidth: true
            radius: 12
            color: "#13243d"
            border.color: "#3a567e"

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 14
                spacing: 8

                Label {
                    text: "USB Capture"
                    color: "#eef6ff"
                    font.pixelSize: 26
                    font.bold: true
                }

                Label {
                    text: usbController.statusText
                    color: "#99d0ff"
                    font.pixelSize: 16
                }

                Label {
                    visible: usbController.captureErrorText.length > 0
                    text: usbController.captureErrorText
                    color: "#ff9f8f"
                    font.pixelSize: 13
                }
            }
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: 12

            Rectangle {
                Layout.fillWidth: true
                radius: 10
                color: "#13243d"
                border.color: "#3a567e"

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 12
                    spacing: 6

                    Label {
                        text: "USBPcap Installed"
                        color: "#8eb1d9"
                    }

                    Label {
                        text: usbController.usbPcapInstalled ? "Yes" : "No"
                        color: usbController.usbPcapInstalled ? "#7df3b2" : "#ff9f8f"
                        font.pixelSize: 24
                        font.bold: true
                    }
                }
            }

            Rectangle {
                Layout.fillWidth: true
                radius: 10
                color: "#13243d"
                border.color: "#3a567e"

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 12
                    spacing: 6

                    Label {
                        text: "Capture State"
                        color: "#8eb1d9"
                    }

                    Label {
                        text: usbController.captureRunning ? "Running" : "Stopped"
                        color: usbController.captureRunning ? "#7df3b2" : "#ffbf69"
                        font.pixelSize: 24
                        font.bold: true
                    }
                }
            }

            Rectangle {
                Layout.fillWidth: true
                radius: 10
                color: "#13243d"
                border.color: "#3a567e"

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 12
                    spacing: 6

                    Label {
                        text: "Parsed Packets"
                        color: "#8eb1d9"
                    }

                    Label {
                        text: usbController.parsedPacketCount.toString()
                        color: "#f4fbff"
                        font.pixelSize: 24
                        font.bold: true
                    }
                }
            }
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: 10

            ComboBox {
                Layout.preferredWidth: 320
                model: usbController.usbPcapUpperFilters
                currentIndex: usbController.selectedFilterIndex
                onActivated: function (index) {
                    usbController.selectFilter(index);
                }
            }

            Button {
                text: "Initialize"
                onClicked: usbController.initialize()
            }

            Button {
                text: "Start Capture"
                enabled: usbController.usbPcapInstalled && !usbController.captureRunning
                onClicked: usbController.startCapture()
            }

            Button {
                text: "Stop"
                enabled: usbController.captureRunning
                onClicked: usbController.stopCapture()
            }

            Button {
                text: "Restart as Administrator"
                visible: usbController.elevationRequired
                onClicked: usbController.restartAsAdministrator()
            }
        }

        RowLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 12

            Rectangle {
                Layout.fillWidth: true
                Layout.fillHeight: true
                radius: 10
                color: "#13243d"
                border.color: "#3a567e"

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 10

                    Label {
                        text: "USBPcap Upper Filters"
                        color: "#dbe8ff"
                        font.pixelSize: 16
                        font.bold: true
                    }

                    ListView {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        clip: true
                        spacing: 8
                        model: usbController.usbPcapUpperFilters

                        delegate: Rectangle {
                            required property int index
                            required property string modelData

                            width: parent.width
                            height: 42
                            radius: 8
                            color: index % 2 === 0 ? "#11243f" : "#142b49"
                            border.color: "#34547b"

                            Text {
                                anchors.verticalCenter: parent.verticalCenter
                                anchors.left: parent.left
                                anchors.leftMargin: 10
                                text: modelData
                                color: "#e8f2ff"
                                font.pixelSize: 14
                            }
                        }
                    }
                }
            }

            Rectangle {
                Layout.fillWidth: true
                Layout.fillHeight: true
                radius: 10
                color: "#13243d"
                border.color: "#3a567e"

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 10

                    Label {
                        text: "Recent Packets"
                        color: "#dbe8ff"
                        font.pixelSize: 16
                        font.bold: true
                    }

                    ListView {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        clip: true
                        spacing: 6
                        model: usbController.recentPackets

                        delegate: Rectangle {
                            required property int index
                            required property string modelData

                            width: parent.width
                            height: 34
                            radius: 6
                            color: index % 2 === 0 ? "#1a2f4f" : "#17304e"

                            Text {
                                anchors.verticalCenter: parent.verticalCenter
                                anchors.left: parent.left
                                anchors.leftMargin: 10
                                text: modelData
                                color: "#f1f7ff"
                                font.pixelSize: 12
                            }
                        }
                    }
                }
            }
        }
    }
}
