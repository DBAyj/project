import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../components" as Components
import "../theme" as Theme

Components.AstraCard {
    id: panel
    property var controller
    implicitHeight: content.implicitHeight + Theme.AstraTheme.spacingLg
    ColumnLayout {
        id: content
        anchors.fill: parent
        spacing: Theme.AstraTheme.spacingXs
        Label { text: "系统状态"; color: Theme.AstraTheme.textPrimary; font.pixelSize: 16; font.bold: true }
        Label { text: panel.controller.systemState.systemVersion + " · " + panel.controller.systemState.cpuArchitecture + " · Qt " + panel.controller.systemState.qtVersion; color: Theme.AstraTheme.textSecondary; font.pixelSize: 12; wrapMode: Text.Wrap }
        RowLayout {
            Layout.fillWidth: true
            Components.AstraStatusBadge { text: "投影 " + panel.controller.systemState.projectionStatus; badgeColor: Theme.AstraTheme.accentProjection }
            Components.AstraStatusBadge { text: "空间 " + panel.controller.systemState.spatialStatus; badgeColor: panel.controller.systemState.spatialStatus === "TRACKING" ? Theme.AstraTheme.statusSuccess : Theme.AstraTheme.statusWarning }
            Components.AstraStatusBadge { text: "审计 " + panel.controller.systemState.auditStatus; badgeColor: panel.controller.systemState.auditStatus === "READY" ? Theme.AstraTheme.statusSuccess : Theme.AstraTheme.statusWarning }
            Components.AstraStatusBadge { text: "配置 " + panel.controller.systemState.configurationStatus; badgeColor: panel.controller.systemState.configurationStatus === "VALID" ? Theme.AstraTheme.statusSuccess : Theme.AstraTheme.statusWarning }
        }
        Label {
            objectName: "configurationWarningLabel"
            Layout.fillWidth: true
            visible: panel.controller.systemState.configurationWarningCount > 0
            text: panel.controller.systemState.configurationWarnings.join("\n")
            color: Theme.AstraTheme.statusWarning
            wrapMode: Text.Wrap
            font.pixelSize: 12
            Accessible.name: "配置警告"
        }
    }
}
