import QtQuick 1.1
import com.nokia.meego 1.0
import "strings.js" as S

// Three ways to add a service: camera, clipboard, typing the key.
Page {
    id: page
    orientationLock: PageOrientation.LockPortrait

    Image { source: "background.png"; y: -36; smooth: false }
    Header { id: header; title: S.t(uiLang, "add") }

    Column {
        anchors { top: header.bottom; left: parent.left; right: parent.right; margins: 24 }
        spacing: 24
        Label {
            width: parent.width
            wrapMode: Text.Wrap
            text: S.t(uiLang, "addHelp")
            color: "#b0b0b0"
            font.pixelSize: 22
        }
        Button {
            width: parent.width
            text: S.t(uiLang, "scan")
            onClicked: pageStack.push(Qt.resolvedUrl("ScanPage.qml"))
        }
        Button {
            width: parent.width
            text: S.t(uiLang, "paste")
            onClicked: {
                var list = store.parse(store.clipboard());
                if (list.length == 0)
                    appWindow.showBanner(S.t(uiLang, store.lastError() == "hotp" ? "hotp" : "badLink"));
                else
                    appWindow.addAccounts(list);
            }
        }
        Button {
            width: parent.width
            text: S.t(uiLang, "manual")
            onClicked: appWindow.openEditor(-1, undefined)
        }
    }

    tools: ToolBarLayout {
        ToolIcon { iconId: "toolbar-back"; onClicked: pageStack.pop() }
    }
}
