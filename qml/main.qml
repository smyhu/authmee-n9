import QtQuick 1.1
import com.nokia.meego 1.0
import com.nokia.extras 1.1
import "strings.js" as S

PageStackWindow {
    id: appWindow
    showStatusBar: true
    initialPage: AccountsPage { id: accounts }

    property color accent: "#eaffd0"
    property color tintHeader: "#373e2e"
    property color tintBorder: "#5a6650"
    property color green: "#a6e36a"
    property color orange: "#ffae3d"
    property color red: "#ff5a4a"

    // corrected time (seconds since 1970), refreshed five times a second
    property real now: clock.now()
    property int nowSec: Math.floor(now)
    property real leftAt: 0

    function remaining(period) {
        return period - (nowSec % period);
    }
    function codeColor(period) {
        var r = remaining(period);
        if (r <= period / 6) return red;
        if (r <= period / 3) return orange;
        return green;
    }
    // Back to the list without animation, then open the editor (avoids two transitions at once).
    function openEditor(index, initial) {
        pageStack.pop(initialPage, true);
        pageStack.push(Qt.resolvedUrl("EditPage.qml"), { index: index, initial: initial });
    }
    // Several accounts from one code (Google Authenticator export) are added at once.
    function addAccounts(list) {
        if (list.length == 1) {
            if (store.contains(list[0]))
                showBanner(S.t(uiLang, "already"));
            else
                openEditor(-1, list[0]);
        } else if (list.length > 1) {
            pageStack.pop(initialPage, true);
            showBanner(S.a(uiLang, "addedN", store.addAll(list)));
        }
    }
    function showBanner(text) {
        banner.text = text;
        banner.show();
    }
    function lockNow() {
        store.lock();
        if (pageStack.currentPage.objectName != "lock")
            pageStack.push(Qt.resolvedUrl("LockPage.qml"), {}, true);
    }

    Timer {
        interval: 200
        repeat: true
        running: Qt.application.active
        triggeredOnStart: true
        onTriggered: {
            var n = clock.now();
            appWindow.now = n;
            var s = Math.floor(n);
            if (s != appWindow.nowSec)
                appWindow.nowSec = s;
        }
    }

    // with a PIN, lock again after a minute in the background
    Connections {
        target: Qt.application
        onActiveChanged: {
            if (!Qt.application.active) {
                appWindow.leftAt = clock.now();
            } else if (store.pinEnabled && !store.locked && appWindow.leftAt > 0 && clock.now() - appWindow.leftAt > 60) {
                appWindow.lockNow();
            }
        }
    }

    InfoBanner {
        id: banner
        y: 36 + 8
    }

    Component.onCompleted: {
        theme.inverted = true;
        try { theme.colorScheme = 4; } catch (e) { }
        if (store.locked)
            pageStack.push(Qt.resolvedUrl("LockPage.qml"), {}, true);
        if (store.setting("clock/atStart", true) == true || store.setting("clock/atStart", true) == "true")
            clock.sync(store.setting("clock/server", "pool.ntp.org"), store.setting("clock/protocol", 0));
    }
}
