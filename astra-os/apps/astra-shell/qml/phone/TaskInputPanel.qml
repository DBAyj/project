import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../components" as Components
import "../theme" as Theme

Components.AstraCard {
    id: panel
    property var controller
    function focusInput() { taskInput.forceActiveFocus() }
    implicitHeight: content.implicitHeight + Theme.AstraTheme.spacingLg
    ColumnLayout {
        id: content
        anchors.fill: parent
        spacing: Theme.AstraTheme.spacingSm
        Label { text: "AI 任务"; color: Theme.AstraTheme.textPrimary; font.pixelSize: 16; font.bold: true }
        Components.AstraTextField {
            id: taskInput
            objectName: "taskInput"
            Layout.fillWidth: true
            placeholderText: "请输入任务，例如：把设备模型投到桌面上"
        }
        RowLayout {
            Layout.fillWidth: true
            spacing: Theme.AstraTheme.spacingSm
            PrivacySelector { id: privacySelector; Layout.fillWidth: true }
            Components.AstraButton {
                id: executeButton
                objectName: "executeButton"
                text: "执行"
                emphasisColor: Theme.AstraTheme.accentAI
                onClicked: {
                    panel.controller.submit(taskInput.text, privacySelector.privacyLevel)
                    if (panel.controller.spatialUIAvailable)
                        panel.controller.createSpatialTaskCard(taskInput.text, panel.controller.taskResult, privacySelector.privacyLevel)
                }
            }
            Components.AstraButton {
                text: "清空"
                emphasisColor: Theme.AstraTheme.surfaceElevated
                onClicked: taskInput.clear()
            }
        }
    }
}
