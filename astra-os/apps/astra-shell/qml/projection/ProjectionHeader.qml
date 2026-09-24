import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../components" as Components
import "../theme" as Theme

Rectangle {
    id: header
    property var controller
    readonly property var spatialState: controller && controller.spatialState ? controller.spatialState : null
    implicitHeight: 54
    color: Theme.AstraTheme.backgroundSecondary
    RowLayout {
        anchors.fill: parent; anchors.margins: Theme.AstraTheme.spacingMd
        Label { text: "投影会话"; color: Theme.AstraTheme.textPrimary; font.bold: true; font.pixelSize: 17; Layout.fillWidth: true }
        Label { text: controller.projectionModel.targetSpace || "等待目标"; color: Theme.AstraTheme.textSecondary; font.pixelSize: 13 }
        Components.AstraPrivacyBadge { privacyLevel: controller.projectionModel.privacyLevel || "PUBLIC" }
        Components.AstraStatusBadge { text: controller.projectionModel.state; badgeColor: controller.projectionModel.modelVisible ? Theme.AstraTheme.accentProjection : Theme.AstraTheme.statusWarning }
        Components.AstraStatusBadge { objectName: "projectionSpatialTarget"; text: header.spatialState ? header.spatialState.targetState : "UNAVAILABLE"; badgeColor: header.spatialState && header.spatialState.targetState === "CALIBRATED" ? Theme.AstraTheme.statusSuccess : Theme.AstraTheme.statusWarning }
    }
}
