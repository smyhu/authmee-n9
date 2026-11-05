import QtQuick 1.1
import com.nokia.meego 1.0
import "strings.js" as S

// New service (index -1, maybe filled from a QR code) or editing an existing one.
Page {
    id: page
    orientationLock: PageOrientation.LockPortrait
    property int index: -1
    property variant initial
    property bool showKey: index < 0
    property string algorithm: "SHA1"
    property int digits: 6
    property int period: 30

    function current() {
        return { "issuer": issuer.text, "name": account.text, "secret": secret.text,
                 "algorithm": page.algorithm, "digits": page.digits, "period": page.period };
    }
    function grouped(c) {
        if (c.length == 6 || c.length == 7) return c.substr(0, 3) + " " + c.substr(3);
        if (c.length == 8) return c.substr(0, 4) + " " + c.substr(4);
        return c;
    }
    function save() {
        if (!store.validSecret(secret.text)) {
            appWindow.showBanner(S.t(uiLang, "badKey"));
            return;
        }
        if (issuer.text == "" && account.text == "") {
            appWindow.showBanner(S.t(uiLang, "needName"));
            return;
        }
        var m = current();
        if (page.index < 0) {
            if (store.contains(m)) {
                appWindow.showBanner(S.t(uiLang, "already"));
                return;
            }
            store.add(m);
        } else {
            store.update(page.index, m);
        }
        appWindow.showBanner(S.t(uiLang, "saved"));
        pageStack.pop();
    }

    Component.onCompleted: {
        var m = page.index >= 0 ? store.get(page.index) : page.initial;
        if (m) {
            issuer.text = m.issuer;
            account.text = m.name;
            secret.text = m.secret;
            page.algorithm = m.algorithm;
            page.digits = m.digits;
            page.period = m.period;
        }
    }

    Image { source: "background.png"; y: -36; smooth: false }
    Header { id: header; title: S.t(uiLang, page.index < 0 ? "newService" : "editService") }

    Flickable {
        id: flick
        anchors { top: header.bottom; left: parent.left; right: parent.right; bottom: parent.bottom }
        contentHeight: col.height + 32
        clip: true
        Column {
            id: col
            anchors { left: parent.left; right: parent.right; margins: 16 }
            y: 12
            spacing: 10

            Label { text: S.t(uiLang, "issuer"); color: "#b0b0b0"; font.pixelSize: 22 }
            TextField {
                id: issuer
                width: parent.width
                inputMethodHints: Qt.ImhNoPredictiveText
            }
            Label { text: S.t(uiLang, "account"); color: "#b0b0b0"; font.pixelSize: 22 }
            TextField {
                id: account
                width: parent.width
                inputMethodHints: Qt.ImhNoPredictiveText | Qt.ImhNoAutoUppercase
            }
            Label { text: S.t(uiLang, "secret"); color: "#b0b0b0"; font.pixelSize: 22 }
            TextField {
                id: secret
                width: parent.width
                echoMode: page.showKey ? TextInput.Normal : TextInput.Password
                inputMethodHints: Qt.ImhNoPredictiveText | Qt.ImhUppercaseOnly | Qt.ImhNoAutoUppercase
                errorHighlight: text != "" && !store.validSecret(text)
            }
            Button {
                visible: page.index >= 0
                text: S.t(uiLang, page.showKey ? "hideKey" : "showKey")
                onClicked: page.showKey = !page.showKey
            }

            Item {
                width: parent.width
                height: 84
                visible: store.validSecret(secret.text)
                Label {
                    anchors { left: parent.left; verticalCenter: parent.verticalCenter }
                    text: S.t(uiLang, "preview")
                    color: "#b0b0b0"
                    font.pixelSize: 22
                }
                Text {
                    anchors { right: parent.right; verticalCenter: parent.verticalCenter }
                    text: secret.text, grouped(store.preview(page.current(), appWindow.nowSec))
                    color: appWindow.codeColor(page.period)
                    font.pixelSize: 38
                    font.letterSpacing: 2
                }
            }

            SectionLabel { text: S.t(uiLang, "advanced") }
            Label { text: S.t(uiLang, "algorithm"); color: "white" }
            ButtonRow {
                width: parent.width
                Button { text: "SHA1"; checked: page.algorithm == "SHA1"; onClicked: page.algorithm = "SHA1" }
                Button { text: "SHA256"; checked: page.algorithm == "SHA256"; onClicked: page.algorithm = "SHA256" }
                Button { text: "SHA512"; checked: page.algorithm == "SHA512"; onClicked: page.algorithm = "SHA512" }
            }
            Label { text: S.t(uiLang, "digits"); color: "white" }
            ButtonRow {
                width: parent.width
                Button { text: "6"; checked: page.digits == 6; onClicked: page.digits = 6 }
                Button { text: "7"; checked: page.digits == 7; onClicked: page.digits = 7 }
                Button { text: "8"; checked: page.digits == 8; onClicked: page.digits = 8 }
            }
            Label { text: S.t(uiLang, "period"); color: "white" }
            ButtonRow {
                width: parent.width
                Button { text: "30 s"; checked: page.period == 30; onClicked: page.period = 30 }
                Button { text: "60 s"; checked: page.period == 60; onClicked: page.period = 60 }
            }
        }
    }
    ScrollDecorator { flickableItem: flick }

    tools: ToolBarLayout {
        ToolIcon { iconId: "toolbar-back"; onClicked: pageStack.pop() }
        ToolButton {
            text: S.t(uiLang, "save")
            anchors.centerIn: parent
            onClicked: page.save()
        }
    }
}
