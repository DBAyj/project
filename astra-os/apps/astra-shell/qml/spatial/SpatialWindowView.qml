import QtQuick
import "../components" as Components

Components.AstraSpatialPanel {
    id: windowView
    property string windowId: ""
    property bool focused: false
    property string privacyLevel: "PUBLIC"
    signal inputRequested(string eventType, string sourceType)
    activeFocusOnTab: true
    focus: focused
    onActiveFocusChanged: {
        if (activeFocus && !focused) inputRequested("SYSTEM_FOCUS", "KEYBOARD")
    }
    Accessible.role: Accessible.Window
    Keys.onPressed: event => {
        if (event.key === Qt.Key_Tab || event.key === Qt.Key_Backtab) {
            event.accepted = false
            return
        }
        inputRequested("KEY_PRESS", "KEYBOARD")
        event.accepted = true
    }
    Behavior on opacity { NumberAnimation { duration: AstraSpatialTheme.animationDuration } }
    title: "空间窗口"
    subtitle: windowId
    implicitWidth: 360
    implicitHeight: 220
    SpatialPrivacyBadge { anchors.right: parent.right; anchors.top: parent.top; privacyLevel: windowView.privacyLevel }
    SpatialFocusIndicator { anchors.fill: parent; focused: windowView.focused }
    TapHandler {
        onTapped: windowView.inputRequested("SYSTEM_FOCUS", "MOUSE")
    }
}
