import QtQuick 1.1
import com.nokia.meego 1.0
import AuthMee 1.0

// Page title bar: dark green gradient fading into the background.
// With showRing, the shared countdown for 30-second codes sits on the right.
Rectangle {
    id: header
    property string title: ""
    property bool showRing: false
    width: parent.width
    height: 72
    gradient: Gradient {
        GradientStop { position: 0.0; color: appWindow.tintHeader }
        GradientStop { position: 1.0; color: "#000000" }
    }
    // thin light edge at the top, as in the N9 title bars
    Rectangle { width: parent.width; height: 2; color: "#4a5340" }
    Label {
        text: parent.title
        color: "white"
        font.pixelSize: 32
        anchors { left: parent.left; leftMargin: 16; right: ring.left; rightMargin: 12; verticalCenter: parent.verticalCenter }
        elide: Text.ElideRight
    }
    Ring {
        id: ring
        visible: header.showRing
        width: visible ? 40 : 0
        height: 40
        anchors { right: parent.right; rightMargin: 20; verticalCenter: parent.verticalCenter }
        lineWidth: 6
        value: (30 - (appWindow.now % 30)) / 30
        color: appWindow.codeColor(30)
        trackColor: "#30ffffff"
        // Seconds inside the ring, switched off at Karol's request (05.11.2026).
        // To bring them back: make the ring 50x50 with lineWidth 5 and uncomment.
        // Text {
        //     anchors.centerIn: parent
        //     text: appWindow.remaining(30)
        //     color: ring.color
        //     font.pixelSize: 20
        //     font.bold: true
        // }
    }
}
