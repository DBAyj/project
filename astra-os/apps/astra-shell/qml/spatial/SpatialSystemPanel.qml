import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../components" as Components

Components.AstraSpatialPanel {
    id: systemPanel
    property var spatialModel
    title: "Spatial UI Runtime"
    subtitle: spatialModel ? spatialModel.p4ReleaseStatus : "P4_RELEASE_BASELINE_FINAL"
    implicitWidth: 340
    implicitHeight: 170
    GridLayout {
        anchors.fill: parent
        columns: 2
        columnSpacing: 18
        rowSpacing: 8
        Label { text: "状态"; color: AstraSpatialTheme.textMuted }
        Label { text: systemPanel.spatialModel ? systemPanel.spatialModel.status : "OFFLINE"; color: systemPanel.spatialModel && systemPanel.spatialModel.status === "READY" ? AstraSpatialTheme.success : AstraSpatialTheme.warning }
        Label { text: "组件"; color: AstraSpatialTheme.textMuted }
        Label { text: systemPanel.spatialModel ? systemPanel.spatialModel.componentCount : 0; color: AstraSpatialTheme.text }
        Label { text: "窗口"; color: AstraSpatialTheme.textMuted }
        Label { text: systemPanel.spatialModel ? systemPanel.spatialModel.windowCount : 0; color: AstraSpatialTheme.text }
        Label { text: "投影安全"; color: AstraSpatialTheme.textMuted }
        Label { text: systemPanel.spatialModel && systemPanel.spatialModel.projectionSafe ? "SAFE" : "ACTIVE"; color: AstraSpatialTheme.projectionAccent }
    }
}
