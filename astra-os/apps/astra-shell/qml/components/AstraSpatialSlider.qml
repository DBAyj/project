import QtQuick
import QtQuick.Controls
import "../spatial" as Spatial

Slider {
    id: control
    implicitHeight: 40
    activeFocusOnTab: true
    Accessible.name: objectName.length > 0 ? objectName : "空间参数"
    Accessible.description: "当前值 " + Math.round(value * 100) / 100
    Accessible.role: Accessible.Slider
    background: Rectangle { x: control.leftPadding; y: control.topPadding + control.availableHeight / 2 - 2; width: control.availableWidth; height: 4; radius: 2; color: Spatial.AstraSpatialTheme.surfaceRaised }
    handle: Rectangle { x: control.leftPadding + control.visualPosition * (control.availableWidth - width); y: control.topPadding + control.availableHeight / 2 - height / 2; width: 18; height: 18; radius: 9; color: Spatial.AstraSpatialTheme.accent; border.width: control.activeFocus ? 2 : 0; border.color: Spatial.AstraSpatialTheme.focusOutline }
}
