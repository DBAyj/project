import QtQuick
import QtQuick.Controls
import "../spatial" as Spatial

Button {
    id: control
    property color accentColor: Spatial.AstraSpatialTheme.accent
    implicitWidth: Math.max(92, contentItem.implicitWidth + 28)
    implicitHeight: 40
    activeFocusOnTab: true
    Accessible.name: text
    Accessible.description: text
    Accessible.role: Accessible.Button
    contentItem: Label {
        text: control.text
        color: control.enabled ? Spatial.AstraSpatialTheme.background : Spatial.AstraSpatialTheme.textMuted
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
        font.pixelSize: 14
        font.weight: Font.DemiBold
    }
    background: Rectangle {
        radius: Spatial.AstraSpatialTheme.radius
        color: control.down ? Qt.darker(control.accentColor, 1.22) : control.accentColor
        border.width: control.activeFocus ? 2 : 0
        border.color: Spatial.AstraSpatialTheme.focusOutline
    }
}
