import QtQuick
import "../spatial" as Spatial

Rectangle {
    id: card
    default property alias content: body.data
    property alias contentItem: body
    color: Spatial.AstraSpatialTheme.surface
    radius: Spatial.AstraSpatialTheme.radius
    border.width: 1
    border.color: Spatial.AstraSpatialTheme.surfaceRaised
    Item {
        id: body
        anchors.fill: parent
        anchors.margins: Spatial.AstraSpatialTheme.spacing
    }
}
