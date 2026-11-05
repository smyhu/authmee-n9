import QtQuick 1.1
import com.nokia.meego 1.0
import "strings.js" as S

// mode "set": (current PIN if any) -> new -> repeat;  mode "remove": current PIN.
Page {
    id: page
    orientationLock: PageOrientation.LockPortrait
    property string mode: "set"
    property string step: store.pinEnabled ? "current" : "new"
    property string first: ""

    Image { source: "background.png"; y: -36; smooth: false }
    Header { id: header; title: S.t(uiLang, page.mode == "remove" ? "usePin" : (store.pinEnabled ? "changePin" : "usePin")) }

    PinPad {
        id: pad
        anchors { top: header.bottom; topMargin: 50; horizontalCenter: parent.horizontalCenter }
        prompt: S.t(uiLang, page.step == "current" ? "currentPin" : page.step == "new" ? "newPin" : "repeatPin")
        onEntered: {
            if (page.step == "current") {
                if (!store.checkPin(pin)) {
                    pad.shake();
                    pad.message = S.t(uiLang, "wrongPin");
                    return;
                }
                if (page.mode == "remove") {
                    store.setPin("");
                    appWindow.showBanner(S.t(uiLang, "pinRemoved"));
                    pageStack.pop();
                    return;
                }
                page.step = "new";
                pad.clear();
            } else if (page.step == "new") {
                page.first = pin;
                page.step = "repeat";
                pad.clear();
            } else {
                if (pin != page.first) {
                    page.step = "new";
                    pad.shake();
                    pad.message = S.t(uiLang, "pinMismatch");
                    return;
                }
                store.setPin(pin);
                appWindow.showBanner(S.t(uiLang, "pinSet"));
                pageStack.pop();
            }
        }
    }

    tools: ToolBarLayout {
        ToolIcon { iconId: "toolbar-back"; onClicked: pageStack.pop() }
    }
}
