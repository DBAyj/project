import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../components" as Components
import "../theme" as Theme

Components.AstraCard {
    id: card
    property var controller
    implicitHeight: content.implicitHeight + Theme.AstraTheme.spacingLg
    ColumnLayout {
        id: content
        anchors.fill: parent
        spacing: Theme.AstraTheme.spacingXs
        RowLayout {
            Layout.fillWidth: true
            Label { text: "当前任务"; color: Theme.AstraTheme.textPrimary; font.pixelSize: 16; font.bold: true; Layout.fillWidth: true }
            Components.AstraStatusBadge {
                text: card.controller.currentTaskStatus
                badgeColor: card.controller.lastErrorCode === 0 ? Theme.AstraTheme.statusSuccess : Theme.AstraTheme.statusError
            }
        }
        Label { Layout.fillWidth: true; text: card.controller.currentRawText.length > 0 ? card.controller.currentRawText : "等待输入"; color: Theme.AstraTheme.textPrimary; wrapMode: Text.Wrap; font.pixelSize: 14 }
        Label { text: "意图: " + card.controller.currentIntent + " · 目标: " + card.controller.currentTargetSpace; color: Theme.AstraTheme.textSecondary; font.pixelSize: 12; wrapMode: Text.Wrap }
        RowLayout {
            Layout.fillWidth: true
            Components.AstraPrivacyBadge { privacyLevel: card.controller.currentPrivacyLevel }
            Label { Layout.fillWidth: true; text: "请求: " + card.controller.currentRequestId; color: Theme.AstraTheme.textSecondary; font.pixelSize: 11; elide: Text.ElideRight; horizontalAlignment: Text.AlignRight }
        }
        Components.AstraErrorBanner { Layout.fillWidth: true; message: card.controller.errorMessage }
    }
}
