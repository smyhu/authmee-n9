import QtQuick 1.1
import com.nokia.meego 1.0
import AuthMee 1.0
import "strings.js" as S

// Camera viewfinder with a QR frame. The camera runs only while this page is shown.
Page {
    id: page
    orientationLock: PageOrientation.LockPortrait

    QrScanner {
        id: scanner
        anchors.fill: parent
        active: page.status == PageStatus.Active && Qt.application.active
        onDecoded: {
            var list = store.parse(text);
            if (list.length == 0) {
                appWindow.showBanner(S.t(uiLang, store.lastError() == "hotp" ? "hotp" : "notOtp"));
                return;
            }
            appWindow.addAccounts(list);
        }
        onCameraError: appWindow.showBanner(S.a(uiLang, "cameraError", message))
    }

    // darkened edges around the square the code should fit in
    Item {
        id: frame
        width: 340; height: 340
        anchors { centerIn: parent; verticalCenterOffset: -30 }
        Repeater {
            model: 4
            Item {
                anchors.fill: parent
                rotation: index * 90
                Rectangle { x: 0; y: 0; width: 56; height: 6; color: appWindow.accent }
                Rectangle { x: 0; y: 0; width: 6; height: 56; color: appWindow.accent }
            }
        }
    }
    Rectangle { anchors { left: parent.left; right: parent.right; top: parent.top; bottom: frame.top } color: "#99000000" }
    Rectangle { anchors { left: parent.left; right: parent.right; top: frame.bottom; bottom: parent.bottom } color: "#99000000" }
    Rectangle { anchors { left: parent.left; right: frame.left; top: frame.top; bottom: frame.bottom } color: "#99000000" }
    Rectangle { anchors { left: frame.right; right: parent.right; top: frame.top; bottom: frame.bottom } color: "#99000000" }

    Label {
        anchors { top: frame.bottom; topMargin: 28; horizontalCenter: parent.horizontalCenter }
        width: parent.width - 64
        horizontalAlignment: Text.AlignHCenter
        wrapMode: Text.Wrap
        text: S.t(uiLang, "scanHint")
        color: "white"
        font.pixelSize: 22
    }
    MouseArea {
        anchors.fill: parent
        onClicked: scanner.focus()
    }

    tools: ToolBarLayout {
        ToolIcon { iconId: "toolbar-back"; onClicked: pageStack.pop() }
    }
}
