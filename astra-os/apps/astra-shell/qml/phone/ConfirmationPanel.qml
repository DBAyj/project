import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../components" as Components
import "../theme" as Theme

Components.AstraCard {
    id: panel
    objectName: "confirmationPanel"
    property var controller
    signal reenterRequested()
    readonly property var result: controller.intentResult
    visible: result.requiresConfirmation
    implicitHeight: visible ? content.implicitHeight + Theme.AstraTheme.spacingLg : 0
    Accessible.name: "意图确认"
    Accessible.description: result.confirmationMessage
    Accessible.role: Accessible.Pane
    ColumnLayout {
        id: content
        anchors.fill: parent
        spacing: Theme.AstraTheme.spacingSm
        Label { text: "需要确认"; color: Theme.AstraTheme.statusWarning; font.pixelSize: 15; font.bold: true }
        Label {
            Layout.fillWidth: true
            text: panel.result.confirmationMessage
            color: Theme.AstraTheme.textPrimary
            font.pixelSize: 13
            wrapMode: Text.Wrap
        }
        RowLayout {
            Layout.fillWidth: true
            spacing: Theme.AstraTheme.spacingSm
            Components.AstraButton {
                objectName: "confirmIntentButton"
                text: "确认"
                emphasisColor: Theme.AstraTheme.statusSuccess
                onClicked: panel.controller.confirmIntent()
            }
            Components.AstraButton {
                objectName: "rejectIntentButton"
                text: "拒绝"
                emphasisColor: Theme.AstraTheme.statusError
                onClicked: panel.controller.rejectIntent()
            }
            Components.AstraButton {
                objectName: "reenterIntentButton"
                text: "重新输入"
                emphasisColor: Theme.AstraTheme.surfaceElevated
                onClicked: panel.reenterRequested()
            }
        }
    }
}
