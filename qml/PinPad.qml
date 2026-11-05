import QtQuick 1.1
import com.nokia.meego 1.0

// Numeric keypad with dots for the typed digits (4-8 digits).
Column {
    id: pad
    property string prompt: ""
    property string message: ""
    property bool enabledPad: true
    property string pin: ""
    property int minLength: 4
    property int maxLength: 8
    signal entered(string pin)
    width: 420
    spacing: 18

    function clear() { pin = ""; }
    function shake() { pin = ""; shakeAnim.restart(); }

    Label {
        anchors.horizontalCenter: parent.horizontalCenter
        text: pad.prompt
        color: "white"
        font.pixelSize: 28
    }
    Row {
        id: dots
        anchors.horizontalCenter: parent.horizontalCenter
        height: 28
        spacing: 16
        Repeater {
            model: Math.max(pad.minLength, pad.pin.length)
            Rectangle {
                width: 20; height: 20; radius: 10
                anchors.verticalCenter: parent.verticalCenter
                color: index < pad.pin.length ? appWindow.accent : "transparent"
                border.color: appWindow.tintBorder
                border.width: 2
            }
        }
        SequentialAnimation {
            id: shakeAnim
            NumberAnimation { target: dots; property: "anchors.horizontalCenterOffset"; to: -18; duration: 50 }
            NumberAnimation { target: dots; property: "anchors.horizontalCenterOffset"; to: 18; duration: 80 }
            NumberAnimation { target: dots; property: "anchors.horizontalCenterOffset"; to: -10; duration: 70 }
            NumberAnimation { target: dots; property: "anchors.horizontalCenterOffset"; to: 0; duration: 60 }
        }
    }
    Item {
        width: parent.width
        height: 30
        Label {
            anchors.centerIn: parent
            text: pad.message
            color: appWindow.red
            font.pixelSize: 22
        }
    }
    Grid {
        anchors.horizontalCenter: parent.horizontalCenter
        columns: 3
        spacing: 14
        Repeater {
            model: ["1", "2", "3", "4", "5", "6", "7", "8", "9", "<", "0", "OK"]
            Rectangle {
                width: 124; height: 88; radius: 14
                color: key.pressed ? "#3a4630" : "#1a1d17"
                border.color: "#2f3729"
                border.width: 1
                opacity: pad.enabledPad && (modelData != "OK" || pad.pin.length >= pad.minLength) ? 1 : 0.35
                Text {
                    anchors.centerIn: parent
                    text: modelData == "<" ? "⌫" : modelData
                    color: modelData == "OK" ? appWindow.accent : "white"
                    font.pixelSize: modelData == "OK" ? 30 : 40
                }
                MouseArea {
                    id: key
                    anchors.fill: parent
                    enabled: pad.enabledPad
                    onClicked: {
                        if (modelData == "<") {
                            pad.pin = pad.pin.substr(0, pad.pin.length - 1);
                        } else if (modelData == "OK") {
                            if (pad.pin.length >= pad.minLength)
                                pad.entered(pad.pin);
                        } else if (pad.pin.length < pad.maxLength) {
                            pad.message = "";
                            pad.pin += modelData;
                        }
                    }
                }
            }
        }
    }
}
