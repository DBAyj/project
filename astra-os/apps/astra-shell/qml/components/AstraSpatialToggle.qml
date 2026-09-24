import QtQuick
import QtQuick.Controls
import "../spatial" as Spatial

Switch {
    id: control
    activeFocusOnTab: true
    Accessible.name: text
    Accessible.description: checked ? "已开启" : "已关闭"
    Accessible.role: Accessible.CheckBox
    contentItem: Label { leftPadding: control.indicator.width + 10; text: control.text; color: Spatial.AstraSpatialTheme.text; verticalAlignment: Text.AlignVCenter }
}
