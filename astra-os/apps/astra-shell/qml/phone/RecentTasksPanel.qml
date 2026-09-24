import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../components" as Components
import "../theme" as Theme

Components.AstraCard {
    id: panel
    property var controller
    implicitHeight: 212
    ColumnLayout {
        anchors.fill: parent
        spacing: Theme.AstraTheme.spacingSm
        Label { text: "最近任务"; color: Theme.AstraTheme.textPrimary; font.pixelSize: 16; font.bold: true }
        ListView {
            id: recentList
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            model: panel.controller.recentTasks
            spacing: Theme.AstraTheme.spacingXs
            delegate: Rectangle {
                required property string rawText
                required property string intent
                required property string executionStatus
                required property string privacyLevel
                required property string timestamp
                width: recentList.width
                height: 48
                color: Theme.AstraTheme.backgroundSecondary
                radius: Theme.AstraTheme.radiusSm
                RowLayout {
                    anchors.fill: parent
                    anchors.margins: Theme.AstraTheme.spacingSm
                    ColumnLayout { Layout.fillWidth: true; spacing: 0
                        Label { text: rawText; color: Theme.AstraTheme.textPrimary; font.pixelSize: 13; elide: Text.ElideRight; Layout.fillWidth: true }
                        Label { text: intent + " · " + timestamp; color: Theme.AstraTheme.textSecondary; font.pixelSize: 11; elide: Text.ElideRight; Layout.fillWidth: true }
                    }
                    Components.AstraStatusBadge { text: executionStatus; badgeColor: executionStatus === "FAILED" || executionStatus === "DENIED" ? Theme.AstraTheme.statusError : Theme.AstraTheme.statusSuccess }
                }
            }
            Label {
                anchors.centerIn: parent
                visible: recentList.count === 0
                text: "暂无任务记录"
                color: Theme.AstraTheme.textSecondary
            }
        }
    }
}
