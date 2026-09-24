import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../components" as Components
import "../theme" as Theme

Components.AstraCard {
    id: panel
    property var controller
    readonly property var spatialState: controller && controller.spatialState ? controller.spatialState : null
    implicitHeight: content.implicitHeight + Theme.AstraTheme.spacingLg

    ColumnLayout {
        id: content
        anchors.fill: parent
        spacing: Theme.AstraTheme.spacingSm
        Label { text: "空间感知"; color: Theme.AstraTheme.textPrimary; font.pixelSize: 16; font.bold: true }
        RowLayout {
            Layout.fillWidth: true
            Components.AstraStatusBadge { objectName: "spatialStateBadge"; text: panel.spatialState ? panel.spatialState.state : "OFFLINE"; badgeColor: panel.spatialState && panel.spatialState.state === "TRACKING" ? Theme.AstraTheme.statusSuccess : Theme.AstraTheme.statusWarning }
            Components.AstraStatusBadge { objectName: "spatialSourceBadge"; text: panel.spatialState ? panel.spatialState.inputSource : "UNAVAILABLE"; badgeColor: Theme.AstraTheme.accentPrimary }
            Components.AstraStatusBadge { objectName: "spatialTargetBadge"; text: panel.spatialState ? panel.spatialState.targetState : "UNAVAILABLE"; badgeColor: Theme.AstraTheme.accentProjection }
        }
        Label {
            objectName: "spatialWarning"
            Layout.fillWidth: true
            visible: panel.spatialState ? panel.spatialState.warning.length > 0 : false
            text: panel.spatialState ? panel.spatialState.warning : ""
            color: Theme.AstraTheme.statusWarning
            wrapMode: Text.Wrap
            font.pixelSize: 12
        }
        RowLayout {
            Layout.fillWidth: true
            spacing: Theme.AstraTheme.spacingXs
            Components.AstraButton { objectName: "spatialStartButton"; text: "启动"; emphasisColor: Theme.AstraTheme.accentPrimary; onClicked: panel.controller.startSpatial() }
            Components.AstraButton { objectName: "spatialDetectButton"; text: "检测"; emphasisColor: Theme.AstraTheme.surfaceElevated; onClicked: panel.controller.detectSpatial() }
            Components.AstraButton { objectName: "spatialSelectButton"; text: "选择"; emphasisColor: Theme.AstraTheme.surfaceElevated; onClicked: panel.controller.selectSpatial() }
            Components.AstraButton { objectName: "spatialCalibrateButton"; text: "标定"; emphasisColor: Theme.AstraTheme.surfaceElevated; onClicked: panel.controller.calibrateSpatial() }
            Components.AstraButton { objectName: "spatialResetButton"; text: "重置"; emphasisColor: Theme.AstraTheme.statusWarning; onClicked: panel.controller.resetSpatial() }
        }
    }
}
