import QtQuick 1.1
import com.nokia.meego 1.0
import "strings.js" as S

// Asks for the PIN; after 5 wrong tries the keypad waits 30 seconds.
Page {
    id: page
    objectName: "lock"
    orientationLock: PageOrientation.LockPortrait
    property int failures: 0
    property int wait: 0

    Image { source: "background.png"; y: -36; smooth: false }
    Header { id: header; title: "AuthMee" }

    PinPad {
        id: pad
        anchors { top: header.bottom; topMargin: 50; horizontalCenter: parent.horizontalCenter }
        prompt: S.t(uiLang, "enterPin")
        enabledPad: page.wait == 0
        onEntered: {
            if (store.unlock(pin)) {
                page.failures = 0;
                pageStack.pop(null, true);
                return;
            }
            page.failures += 1;
            pad.shake();
            if (page.failures >= 5) {
                page.wait = 30;
                waitTimer.start();
            } else {
                pad.message = S.t(uiLang, "wrongPin");
            }
        }
    }
    Timer {
        id: waitTimer
        interval: 1000
        repeat: true
        onTriggered: {
            page.wait -= 1;
            pad.message = page.wait > 0 ? S.a(uiLang, "waitN", page.wait) : "";
            if (page.wait <= 0) {
                stop();
                page.failures = 3;
            }
        }
    }
    onWaitChanged: if (wait > 0) pad.message = S.a(uiLang, "waitN", wait)
}
