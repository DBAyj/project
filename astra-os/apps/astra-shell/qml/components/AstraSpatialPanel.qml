import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../spatial" as Spatial

Rectangle {
    id: panel
    property string title: ""
    property string subtitle: ""
    default property alias content: contentArea.data
    color: Spatial.AstraSpatialTheme.surface
    radius: Spatial.AstraSpatialTheme.radius
    border.width: 1
    border.color: Spatial.AstraSpatialTheme.surfaceRaised
    Accessible.name: title
    Accessible.description: subtitle
    ColumnLayout {
        anchors.fill: parent
        anchors.margins: Spatial.AstraSpatialTheme.spacing
        spacing: 10
        Label { text: panel.title; color: Spatial.AstraSpatialTheme.text; font.pixelSize: 15; font.weight: Font.DemiBold }
        Label { visible: panel.subtitle.length > 0; text: panel.subtitle; color: Spatial.AstraSpatialTheme.textMuted; font.pixelSize: 12; wrapMode: Text.Wrap; Layout.fillWidth: true }
        Item { id: contentArea; Layout.fillWidth: true; Layout.fillHeight: true }
    }
}
