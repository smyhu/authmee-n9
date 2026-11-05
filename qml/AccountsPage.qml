import QtQuick 1.1
import com.nokia.meego 1.0
import AuthMee 1.0
import "strings.js" as S

// The list of services with their current codes. Tap copies a code,
// hold opens edit / move / remove. Pull the list down at its top to search.
Page {
    id: page
    orientationLock: PageOrientation.LockPortrait
    property bool searching: false
    property int selected: -1
    property string selectedName: ""
    property bool barAnimated: true

    function reload() {
        model.clear();
        if (store.locked)
            return;
        var q = searching ? S.fold(search.text) : "";
        for (var i = 0; i < store.count; ++i) {
            var a = store.get(i);
            if (q != "" && S.fold(a.issuer + " " + a.name).indexOf(q) < 0)
                continue;
            model.append({ "idx": i, "issuer": a.issuer, "account": a.name, "period": a.period });
        }
    }
    function grouped(c) {
        if (c.length == 6) return c.substr(0, 3) + " " + c.substr(3);
        if (c.length == 8) return c.substr(0, 4) + " " + c.substr(4);
        if (c.length == 7) return c.substr(0, 3) + " " + c.substr(3);
        return c;
    }
    function openSearch() {
        if (searching)
            return;
        closeAnim.stop();
        barAnimated = false;
        searching = true;
        barAnimated = true;
        openAnim.restart();
        search.forceActiveFocus();
        search.platformOpenSoftwareInputPanel();
        idle.restart();
    }
    function closeSearch() {
        if (!searching)
            return;
        idle.stop();
        search.platformCloseSoftwareInputPanel();
        openAnim.stop();
        closeAnim.restart();
    }
    Component.onCompleted: reload()
    Connections { target: store; onChanged: page.reload() }

    Image { source: "background.png"; y: -36; smooth: false }
    ListModel { id: model }
    Header { id: header; title: "AuthMee"; showRing: true }

    Item {
        id: searchBar
        anchors { top: header.bottom; left: parent.left; right: parent.right }
        height: page.searching ? 76 : 0
        clip: true
        Behavior on height {
            enabled: page.barAnimated
            NumberAnimation { duration: 300; easing.type: Easing.OutCubic }
        }
        Item {
            id: searchContent
            width: parent.width
            height: 76
            Rectangle {
                anchors { fill: parent; leftMargin: 16; rightMargin: 16; topMargin: 10; bottomMargin: 10 }
                radius: 12
                color: "#0c0c0c"
            }
            Image {
                id: lens
                anchors { left: parent.left; leftMargin: 28; verticalCenter: parent.verticalCenter }
                source: "image://theme/icon-m-common-search-inverse"
                opacity: 0.7
            }
            TextField {
                id: search
                anchors { left: lens.right; right: clearIcon.left; verticalCenter: parent.verticalCenter }
                inputMethodHints: Qt.ImhNoPredictiveText | Qt.ImhNoAutoUppercase
                platformStyle: TextFieldStyle {
                    background: ""; backgroundSelected: ""; backgroundDisabled: ""; backgroundError: ""
                    textColor: "#ffffff"
                    paddingLeft: 8
                }
                onTextChanged: { page.reload(); idle.restart(); }
                Keys.onReturnPressed: search.platformCloseSoftwareInputPanel()
            }
            Image {
                id: clearIcon
                anchors { right: parent.right; rightMargin: 24; verticalCenter: parent.verticalCenter }
                source: "image://theme/icon-m-input-clear"
                visible: search.text != ""
                width: visible ? implicitWidth : 0
                MouseArea {
                    anchors { fill: parent; margins: -12 }
                    onClicked: { search.text = ""; search.forceActiveFocus(); }
                }
            }
        }
    }
    ParallelAnimation {
        id: openAnim
        NumberAnimation { target: searchContent; property: "opacity"; from: 0; to: 1; duration: 300; easing.type: Easing.OutQuad }
        NumberAnimation { target: searchContent; property: "y"; from: 30; to: 0; duration: 300; easing.type: Easing.OutCubic }
    }
    SequentialAnimation {
        id: closeAnim
        NumberAnimation { target: searchContent; property: "opacity"; to: 0; duration: 200 }
        ScriptAction { script: { search.text = ""; page.searching = false; page.reload(); } }
    }
    Timer {
        id: idle
        interval: 5000
        onTriggered: if (search.text == "") page.closeSearch()
    }

    ListView {
        id: list
        anchors { top: searchBar.bottom; left: parent.left; right: parent.right; bottom: parent.bottom }
        clip: true
        model: model
        property real pullStart: 0
        property bool pulling: false
        onMovementStarted: {
            pullStart = contentY;
            pulling = !page.searching && contentY <= 0;
            if (page.searching)
                idle.restart();
        }
        onMovementEnded: pulling = false
        onContentYChanged: {
            if (!pulling)
                return;
            var distance = pullStart - contentY;
            if (distance >= 50) {
                pulling = false;
                page.openSearch();
                contentY += 76;
            } else if (distance < 0) {
                pulling = false;
            }
        }
        delegate: Item {
            id: row
            width: list.width
            height: 100
            property string code: store.code(model.idx, appWindow.nowSec)
            property color tone: appWindow.codeColor(model.period)
            property bool lastSeconds: appWindow.remaining(model.period) <= model.period / 6
            Rectangle {
                anchors.fill: parent
                color: "#ffffff"
                opacity: mouse.pressed ? 0.12 : 0
            }
            Rectangle {
                anchors { left: parent.left; right: parent.right; bottom: parent.bottom; leftMargin: 16; rightMargin: 16 }
                height: 1
                color: "#1fffffff"
            }
            Column {
                anchors { left: parent.left; right: codeBox.left; leftMargin: 16; rightMargin: 12; verticalCenter: parent.verticalCenter }
                Label {
                    width: parent.width
                    text: model.issuer != "" ? model.issuer : model.account
                    elide: Text.ElideRight
                    font.pixelSize: 26
                    color: "white"
                }
                Label {
                    width: parent.width
                    visible: model.issuer != "" && model.account != ""
                    text: model.account
                    elide: Text.ElideRight
                    font.pixelSize: 20
                    color: "#a0a0a0"
                }
            }
            Column {
                id: codeBox
                anchors { right: parent.right; rightMargin: 16; verticalCenter: parent.verticalCenter }
                Text {
                    anchors.right: parent.right
                    text: page.grouped(row.code)
                    color: row.tone
                    font.pixelSize: 38
                    font.letterSpacing: 1
                }
                // only services with another period than 30 s show their own small ring
                Row {
                    anchors.right: parent.right
                    spacing: 8
                    visible: model.period != 30 || row.lastSeconds
                    height: visible ? 22 : 0
                    Text {
                        visible: row.lastSeconds
                        text: page.grouped(store.code(model.idx, appWindow.nowSec + model.period))
                        color: "#808080"
                        font.pixelSize: 18
                    }
                    Ring {
                        visible: model.period != 30
                        width: 20; height: 20
                        lineWidth: 4
                        value: (model.period - (appWindow.now % model.period)) / model.period
                        color: row.tone
                        trackColor: "#30ffffff"
                    }
                }
            }
            MouseArea {
                id: mouse
                anchors.fill: parent
                onClicked: {
                    store.copy(row.code);
                    appWindow.showBanner(S.t(uiLang, "copied"));
                }
                onPressAndHold: {
                    page.selected = model.idx;
                    page.selectedName = model.issuer != "" ? model.issuer : model.account;
                    itemMenu.open();
                }
            }
        }
    }
    ScrollDecorator { flickableItem: list }

    Label {
        anchors { centerIn: parent; verticalCenterOffset: -40 }
        width: parent.width - 80
        horizontalAlignment: Text.AlignHCenter
        wrapMode: Text.Wrap
        visible: store.count == 0 && !page.searching && !store.locked
        text: S.t(uiLang, "empty")
        color: "#808080"
        font.pixelSize: 26
    }

    ContextMenu {
        id: itemMenu
        MenuLayout {
            MenuItem {
                text: S.t(uiLang, "edit")
                onClicked: pageStack.push(Qt.resolvedUrl("EditPage.qml"), { index: page.selected })
            }
            MenuItem {
                text: S.t(uiLang, "moveUp")
                enabled: page.selected > 0
                onClicked: store.move(page.selected, page.selected - 1)
            }
            MenuItem {
                text: S.t(uiLang, "moveDown")
                enabled: page.selected < store.count - 1
                onClicked: store.move(page.selected, page.selected + 1)
            }
            MenuItem {
                text: S.t(uiLang, "remove")
                onClicked: removeDialog.open()
            }
        }
    }

    QueryDialog {
        id: removeDialog
        titleText: S.t(uiLang, "remove")
        message: S.a(uiLang, "removeAsk", page.selectedName)
        acceptButtonText: S.t(uiLang, "remove")
        rejectButtonText: S.t(uiLang, "cancel")
        onAccepted: {
            store.remove(page.selected);
            appWindow.showBanner(S.t(uiLang, "removed"));
        }
    }

    QueryDialog {
        id: aboutDialog
        titleText: "AuthMee " + appVersion
        message: S.t(uiLang, "aboutText") + "<br><br>" + S.t(uiLang, "credit") +
                 "<br><br>MIT · <a href=\"https://github.com/smyhu/authmee-n9\" style=\"color:" + appWindow.accent +
                 "; text-decoration:underline\">github.com/smyhu/authmee-n9</a>" +
                 "<br><br><small>ZBar (LGPL 2.1) · OpenSSL</small>"
        acceptButtonText: S.t(uiLang, "close")
        onLinkActivated: Qt.openUrlExternally(link)
    }

    Menu {
        id: mainMenu
        MenuLayout {
            MenuItem { text: S.t(uiLang, "settings"); onClicked: pageStack.push(Qt.resolvedUrl("SettingsPage.qml")) }
            MenuItem { text: S.t(uiLang, "about"); onClicked: aboutDialog.open() }
        }
    }

    tools: ToolBarLayout {
        ToolIcon {
            iconId: "toolbar-add"
            onClicked: pageStack.push(Qt.resolvedUrl("AddPage.qml"))
        }
        ToolIcon {
            iconId: "toolbar-search"
            onClicked: page.searching ? page.closeSearch() : page.openSearch()
        }
        ToolIcon {
            iconId: "toolbar-view-menu"
            onClicked: mainMenu.status == DialogStatus.Closed ? mainMenu.open() : mainMenu.close()
        }
    }
}
