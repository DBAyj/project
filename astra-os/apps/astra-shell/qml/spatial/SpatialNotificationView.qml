import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Rectangle {
    id: notification
    property string title: "系统通知"
    property string message: ""
    property string severity: "INFO"
    property var actions: []
    readonly property int actionCount: actionRepeater.count
    signal actionTriggered(string action)
    signal focusRequested()
    function triggerAction(index) {
        if (index >= 0 && index < actions.length) actionTriggered(String(actions[index]))
    }
    implicitWidth: 320
    implicitHeight: content.implicitHeight + 24
    radius: AstraSpatialTheme.radius
    color: AstraSpatialTheme.surfaceRaised
    border.width: severity === "CRITICAL" ? 2 : 1
    border.color: severity === "CRITICAL" || severity === "ERROR" ? AstraSpatialTheme.error : severity === "WARNING" ? AstraSpatialTheme.warning : AstraSpatialTheme.accent
    Accessible.name: title
    Accessible.description: message
    Accessible.role: Accessible.AlertMessage
    ColumnLayout {
        id: content
        anchors.fill: parent
        anchors.margins: 12
        spacing: 4
        Label { text: notification.title; color: AstraSpatialTheme.text; font.weight: Font.DemiBold }
        Label { text: notification.message; color: AstraSpatialTheme.textMuted; wrapMode: Text.Wrap; Layout.fillWidth: true }
        RowLayout {
            visible: notification.actions.length > 0
            Layout.fillWidth: true
            spacing: 6
            Repeater {
                id: actionRepeater
                model: notification.actions
                delegate: Button {
                    required property int index
                    required property var modelData
                    objectName: "notificationAction-" + index
                    text: String(modelData)
                    activeFocusOnTab: true
                    Accessible.name: text
                    Accessible.role: Accessible.Button
                    onActiveFocusChanged: {
                        if (activeFocus) notification.focusRequested()
                    }
                    onClicked: notification.triggerAction(index)
                }
            }
        }
    }
}
