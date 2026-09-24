import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../theme" as Theme

ApplicationWindow {
    id: projection
    property var controller
    width: 1280
    height: 720
    minimumWidth: 800
    minimumHeight: 450
    visible: true
    title: "AstraOS Projection Display"
    color: Theme.AstraTheme.backgroundPrimary
    visibility: controller && controller.projectionFullscreen ? Window.FullScreen : Window.Windowed
    ColumnLayout {
        anchors.fill: parent
        anchors.margins: Theme.AstraTheme.spacingMd
        spacing: Theme.AstraTheme.spacingSm
        ProjectionHeader { Layout.fillWidth: true; controller: projection.controller }
        Item {
            Layout.fillWidth: true; Layout.fillHeight: true
            Image {
                id: p4FixtureOutput
                objectName: "p4FixtureOutput"
                anchors.fill: parent
                visible: Boolean(projection.controller && projection.controller.p4ProjectionActive)
                source: visible && projection.controller ? (projection.controller.p4FramePath || "") : ""
                cache: false
                fillMode: Image.PreserveAspectFit
            }
            ProjectionScene { anchors.fill: parent; controller: projection.controller }
            ProjectionEmptyState { anchors.fill: parent; controller: projection.controller }
        }
        ProjectionControls { Layout.fillWidth: true; controller: projection.controller }
    }
}
