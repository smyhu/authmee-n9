import QtQuick 1.1
import com.nokia.meego 1.0

Item {
    property alias text: label.text
    width: parent.width
    height: 44
    Label {
        id: label
        anchors { left: parent.left; bottom: parent.bottom; bottomMargin: 6 }
        font.pixelSize: 22
        font.bold: true
        color: appWindow.accent
    }
    Rectangle {
        anchors { left: label.right; leftMargin: 12; right: parent.right; verticalCenter: label.verticalCenter }
        height: 1
        color: appWindow.tintBorder
    }
}
