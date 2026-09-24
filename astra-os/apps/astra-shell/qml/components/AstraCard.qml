import QtQuick
import "../theme" as Theme

Rectangle {
    default property alias content: contentItem.data
    property alias contentItem: contentItem
    color: Theme.AstraTheme.surfacePrimary
    radius: Theme.AstraTheme.radiusMd
    border.color: Theme.AstraTheme.surfaceElevated
    border.width: 1
    Item {
        id: contentItem
        anchors.fill: parent
        anchors.margins: Theme.AstraTheme.spacingMd
    }
}
