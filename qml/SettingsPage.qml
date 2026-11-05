import QtQuick 1.1
import com.nokia.meego 1.0
import "strings.js" as S

// PIN, time correction and backups.
Page {
    id: page
    orientationLock: PageOrientation.LockPortrait
    property int protocol: Number(store.setting("clock/protocol", 0))

    function clockText() {
        if (clock.checkedAt == "")
            return S.t(uiLang, "notChecked");
        var o = clock.offset;
        var s = Math.abs(o) < 0.05 ? S.t(uiLang, "exact")
              : S.a(uiLang, o > 0 ? "behind" : "ahead", Math.abs(o).toFixed(1));
        return s + " (" + S.a(uiLang, "checkedAt", clock.checkedAt) + ")";
    }

    Connections {
        target: clock
        onFinished: if (!ok) appWindow.showBanner(S.t(uiLang, "timeFail"))
    }

    Image { source: "background.png"; y: -36; smooth: false }
    Header { id: header; title: S.t(uiLang, "settings") }

    Flickable {
        id: flick
        anchors { top: header.bottom; left: parent.left; right: parent.right; bottom: parent.bottom }
        contentHeight: col.height + 32
        clip: true
        Column {
            id: col
            anchors { left: parent.left; right: parent.right; margins: 16 }
            spacing: 14

            SectionLabel { text: S.t(uiLang, "security") }
            Item {
                width: parent.width; height: 64
                Label { anchors { left: parent.left; right: pinSwitch.left; verticalCenter: parent.verticalCenter }
                        text: S.t(uiLang, "usePin"); wrapMode: Text.Wrap; color: "white" }
                Switch {
                    id: pinSwitch
                    anchors { right: parent.right; verticalCenter: parent.verticalCenter }
                    checked: store.pinEnabled
                    // the switch only opens the PIN page; its state follows the store
                    onCheckedChanged: {
                        if (checked != store.pinEnabled) {
                            pageStack.push(Qt.resolvedUrl("PinPage.qml"), { mode: checked ? "set" : "remove" });
                            checked = store.pinEnabled;
                        }
                    }
                    Connections { target: store; onChanged: pinSwitch.checked = store.pinEnabled }
                }
            }
            Button {
                visible: store.pinEnabled
                width: parent.width
                text: S.t(uiLang, "changePin")
                onClicked: pageStack.push(Qt.resolvedUrl("PinPage.qml"), { mode: "set" })
            }
            Label {
                width: parent.width
                wrapMode: Text.Wrap
                text: S.t(uiLang, "pinHelp")
                color: "#909090"
                font.pixelSize: 20
            }

            SectionLabel { text: S.t(uiLang, "time") }
            Label {
                width: parent.width
                wrapMode: Text.Wrap
                text: clock.checkedAt, clock.offset, page.clockText()
                color: "white"
            }
            Label { text: S.t(uiLang, "server"); color: "#b0b0b0"; font.pixelSize: 22 }
            TextField {
                id: server
                width: parent.width
                text: store.setting("clock/server", "pool.ntp.org")
                inputMethodHints: Qt.ImhUrlCharactersOnly | Qt.ImhNoPredictiveText | Qt.ImhNoAutoUppercase
                onTextChanged: if (text != "") store.setSetting("clock/server", text)
            }
            ButtonRow {
                width: parent.width
                Button { text: "NTP (123)"; checked: page.protocol == 0; onClicked: { page.protocol = 0; store.setSetting("clock/protocol", 0); } }
                Button { text: "TIME (37)"; checked: page.protocol == 1; onClicked: { page.protocol = 1; store.setSetting("clock/protocol", 1); } }
            }
            Item {
                width: parent.width; height: 64
                Button {
                    id: checkButton
                    anchors { left: parent.left; verticalCenter: parent.verticalCenter }
                    width: parent.width - 80
                    text: S.t(uiLang, "checkNow")
                    enabled: !clock.busy
                    onClicked: clock.sync(server.text, page.protocol)
                }
                BusyIndicator {
                    anchors { right: parent.right; rightMargin: 16; verticalCenter: parent.verticalCenter }
                    running: clock.busy
                    visible: clock.busy
                }
            }
            Item {
                width: parent.width; height: 64
                Label { anchors { left: parent.left; right: startSwitch.left; verticalCenter: parent.verticalCenter }
                        text: S.t(uiLang, "checkAtStart"); wrapMode: Text.Wrap; color: "white" }
                Switch {
                    id: startSwitch
                    anchors { right: parent.right; verticalCenter: parent.verticalCenter }
                    checked: String(store.setting("clock/atStart", true)) == "true"
                    onCheckedChanged: store.setSetting("clock/atStart", checked)
                }
            }
            Label {
                width: parent.width
                wrapMode: Text.Wrap
                text: S.t(uiLang, "timeHelp")
                color: "#909090"
                font.pixelSize: 20
            }

            SectionLabel { text: S.t(uiLang, "backup") }
            Button {
                width: parent.width
                text: S.t(uiLang, "saveBackup")
                enabled: store.count > 0
                onClicked: pageStack.push(Qt.resolvedUrl("BackupPage.qml"), { mode: "save" })
            }
            Button {
                width: parent.width
                text: S.t(uiLang, "restoreBackup")
                onClicked: pageStack.push(Qt.resolvedUrl("BackupPage.qml"), { mode: "restore" })
            }
            Label {
                width: parent.width
                wrapMode: Text.Wrap
                text: S.t(uiLang, "backupHelp")
                color: "#909090"
                font.pixelSize: 20
            }
        }
    }
    ScrollDecorator { flickableItem: flick }

    tools: ToolBarLayout {
        ToolIcon { iconId: "toolbar-back"; onClicked: pageStack.pop() }
    }
}
