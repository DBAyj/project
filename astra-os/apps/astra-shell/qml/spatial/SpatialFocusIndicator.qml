import QtQuick

Rectangle {
    property bool focused: false
    color: "transparent"
    radius: AstraSpatialTheme.radius
    border.width: focused ? 2 : 0
    border.color: AstraSpatialTheme.focusOutline
    visible: focused
    Accessible.ignored: true
}
