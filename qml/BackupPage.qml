import QtQuick 1.1
import com.nokia.meego 1.0
import "strings.js" as S

// mode "save": password twice -> encrypted file;  mode "restore": choose file + password.
Page {
    id: page
    orientationLock: PageOrientation.LockPortrait
    property string mode: "save"
    property string chosen: ""
    property variant files: store.backups()

    function fileName(path) {
        return path.substring(path.lastIndexOf("/") + 1);
    }
    function run() {
        if (page.mode == "save") {
            if (pass1.text == "" || pass1.text != pass2.text) {
                appWindow.showBanner(S.t(uiLang, "passwordMismatch"));
                return;
            }
            var path = store.exportBackup(pass1.text);
            appWindow.showBanner(path != "" ? S.a(uiLang, "backupSaved", page.fileName(path)) : S.t(uiLang, "backupFailed"));
            if (path != "")
                pageStack.pop();
        } else {
            var n = store.importBackup(page.chosen, pass1.text);
            if (n < 0) {
                appWindow.showBanner(S.t(uiLang, "wrongPassword"));
                return;
            }
            appWindow.showBanner(S.a(uiLang, "restored", n));
            pageStack.pop();
        }
    }

    Image { source: "background.png"; y: -36; smooth: false }
    Header { id: header; title: S.t(uiLang, page.mode == "save" ? "saveBackup" : "restoreBackup") }

    Flickable {
        id: flick
        anchors { top: header.bottom; left: parent.left; right: parent.right; bottom: parent.bottom }
        contentHeight: col.height + 32
        clip: true
        Column {
            id: col
            anchors { left: parent.left; right: parent.right; margins: 16 }
            y: 12
            spacing: 12

            SectionLabel { visible: page.mode == "restore"; text: S.t(uiLang, "chooseBackup") }
            Label {
                visible: page.mode == "restore" && page.files.length == 0
                width: parent.width
                wrapMode: Text.Wrap
                text: S.a(uiLang, "noBackups", store.backupFolder())
                color: "#909090"
            }
            ButtonColumn {
                visible: page.mode == "restore"
                width: parent.width
                Repeater {
                    model: page.mode == "restore" ? page.files : []
                    Button {
                        text: page.fileName(modelData)
                        checked: page.chosen == modelData
                        onClicked: page.chosen = modelData
                    }
                }
            }

            Label { text: S.t(uiLang, "password"); color: "#b0b0b0"; font.pixelSize: 22 }
            TextField {
                id: pass1
                width: parent.width
                echoMode: TextInput.Password
                inputMethodHints: Qt.ImhNoPredictiveText | Qt.ImhNoAutoUppercase
            }
            Label { visible: page.mode == "save"; text: S.t(uiLang, "repeatPassword"); color: "#b0b0b0"; font.pixelSize: 22 }
            TextField {
                id: pass2
                visible: page.mode == "save"
                width: parent.width
                echoMode: TextInput.Password
                inputMethodHints: Qt.ImhNoPredictiveText | Qt.ImhNoAutoUppercase
            }
            Button {
                width: parent.width
                text: S.t(uiLang, page.mode == "save" ? "save" : "restore")
                enabled: pass1.text != "" && (page.mode == "save" || page.chosen != "")
                onClicked: page.run()
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
