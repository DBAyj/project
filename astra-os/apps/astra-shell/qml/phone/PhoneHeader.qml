import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../components" as Components
import "../theme" as Theme

Rectangle {
    id: header
    property var controller
    implicitHeight: 64
    color: Theme.AstraTheme.backgroundSecondary
    radius: Theme.AstraTheme.radiusMd
    RowLayout {
        anchors.fill: parent
        anchors.margins: Theme.AstraTheme.spacingMd
        spacing: Theme.AstraTheme.spacingSm
        ColumnLayout {
            Layout.fillWidth: true
            spacing: 0
            Label { text: "星穹OS"; color: Theme.AstraTheme.textPrimary; font.pixelSize: 22; font.bold: true }
            Label { text: "开发工作区"; color: Theme.AstraTheme.textSecondary; font.pixelSize: 12 }
        }
        Components.AstraStatusBadge {
            text: header.controller.systemState.projectionStatus
            badgeColor: header.controller.systemState.projectionStatus === "ACTIVE" ? Theme.AstraTheme.accentProjection : Theme.AstraTheme.statusSuccess
        }
        Label {
            id: clock
            color: Theme.AstraTheme.textSecondary
            font.pixelSize: 13
            text: Qt.formatTime(new Date(), "HH:mm")
            Timer { interval: 1000; running: true; repeat: true; onTriggered: clock.text = Qt.formatTime(new Date(), "HH:mm") }
        }
    }
}
