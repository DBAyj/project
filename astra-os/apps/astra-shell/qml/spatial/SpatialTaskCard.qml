import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../components" as Components

Components.AstraSpatialCard {
    id: taskCard
    property string taskTitle: "等待空间任务"
    property string summary: ""
    property string status: "READY"
    property real progress: status === "COMPLETED" ? 1 : 0
    property string intentType: ""
    property real confidence: 0
    property string executionStrategy: ""
    property string privacyLevel: "PRIVATE_SCREEN_ONLY"
    signal confirmRequested()
    signal cancelRequested()
    signal focusRequested()
    implicitWidth: 360
    implicitHeight: content.implicitHeight + 32
    Accessible.name: taskTitle
    Accessible.description: summary + "，状态 " + status + "，意图 " + intentType
    Accessible.role: Accessible.Grouping
    ColumnLayout {
        id: content
        anchors.fill: parent
        spacing: 8
        RowLayout {
            Layout.fillWidth: true
            Label { text: taskCard.taskTitle; color: AstraSpatialTheme.text; font.pixelSize: 15; font.weight: Font.DemiBold; elide: Text.ElideRight; Layout.fillWidth: true }
            SpatialPrivacyBadge { privacyLevel: taskCard.privacyLevel }
        }
        Label { text: taskCard.summary || "空间运行时已就绪"; color: AstraSpatialTheme.textMuted; wrapMode: Text.Wrap; Layout.fillWidth: true; font.pixelSize: 12 }
        Label {
            text: (taskCard.intentType || "未指定意图") + " · 置信度 " + Math.round(taskCard.confidence * 100) + "% · " + (taskCard.executionStrategy || "未指定策略")
            color: AstraSpatialTheme.textMuted
            elide: Text.ElideRight
            Layout.fillWidth: true
            font.pixelSize: 11
        }
        Rectangle { Layout.fillWidth: true; Layout.preferredHeight: 3; radius: 2; color: AstraSpatialTheme.surfaceRaised; Rectangle { width: parent.width * Math.max(0, Math.min(1, taskCard.progress)); height: parent.height; radius: 2; color: AstraSpatialTheme.aiAccent } }
        RowLayout {
            Layout.fillWidth: true
            Label {
                text: taskCard.status === "FAILED" ? "FAILED · 任务执行失败" : taskCard.status
                color: taskCard.status === "FAILED" ? AstraSpatialTheme.error : AstraSpatialTheme.success
                font.pixelSize: 11
                font.weight: Font.DemiBold
                Layout.fillWidth: true
            }
            Components.AstraSpatialButton {
                id: confirmButton
                objectName: "taskConfirmButton"
                text: "确认"
                enabled: taskCard.status === "WAITING_CONFIRMATION"
                implicitWidth: 72
                implicitHeight: 32
                KeyNavigation.tab: cancelButton
                KeyNavigation.priority: KeyNavigation.BeforeItem
                onActiveFocusChanged: {
                    if (activeFocus) taskCard.focusRequested()
                }
                onClicked: taskCard.confirmRequested()
            }
            Components.AstraSpatialButton {
                id: cancelButton
                objectName: "taskCancelButton"
                text: "取消"
                enabled: ["CREATED", "WAITING_CONFIRMATION", "READY", "RUNNING", "PAUSED"].includes(taskCard.status)
                implicitWidth: 72
                implicitHeight: 32
                KeyNavigation.backtab: confirmButton
                KeyNavigation.priority: KeyNavigation.BeforeItem
                onActiveFocusChanged: {
                    if (activeFocus) taskCard.focusRequested()
                }
                onClicked: taskCard.cancelRequested()
            }
        }
    }
}
