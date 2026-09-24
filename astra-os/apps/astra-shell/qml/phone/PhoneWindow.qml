import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../theme" as Theme
import "../spatial" as Spatial

ApplicationWindow {
    id: phone
    property var controller
    width: 430
    height: 860
    minimumWidth: 380
    minimumHeight: 720
    visible: true
    title: "AstraOS Phone Display"
    color: Theme.AstraTheme.backgroundPrimary
    ColumnLayout {
        anchors.fill: parent
        anchors.margins: Theme.AstraTheme.spacingMd
        spacing: Theme.AstraTheme.spacingMd
        PhoneHeader { Layout.fillWidth: true; controller: phone.controller }
        ScrollView {
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            contentWidth: availableWidth
            Column {
                width: parent.width
                spacing: Theme.AstraTheme.spacingMd
                TaskInputPanel { id: taskInputPanel; width: parent.width; controller: phone.controller }
                TaskResultCard { width: parent.width; controller: phone.controller }
                IntentAnalysisCard { width: parent.width; controller: phone.controller }
                ConfirmationPanel {
                    width: parent.width
                    controller: phone.controller
                    onReenterRequested: taskInputPanel.focusInput()
                }
                Spatial.SpatialWorkspace { width: parent.width; controller: phone.controller; displayTarget: "PHONE" }
                RecentTasksPanel { width: parent.width; controller: phone.controller }
                SpatialStatusPanel { width: parent.width; controller: phone.controller }
                SystemStatusPanel { width: parent.width; controller: phone.controller }
            }
        }
    }
}
