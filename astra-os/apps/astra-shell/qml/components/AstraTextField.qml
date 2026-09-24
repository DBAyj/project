import QtQuick
import QtQuick.Controls
import "../theme" as Theme

TextArea {
    id: control
    property string accessibleLabel: "任务输入"
    implicitHeight: 92
    wrapMode: TextEdit.Wrap
    selectByMouse: true
    color: Theme.AstraTheme.textPrimary
    placeholderTextColor: Theme.AstraTheme.textSecondary
    font.pixelSize: 15
    Accessible.name: accessibleLabel
    Accessible.description: "输入要执行的 AstraOS 任务"
    Accessible.role: Accessible.EditableText
    background: Rectangle {
        radius: Theme.AstraTheme.radiusSm
        color: Theme.AstraTheme.backgroundSecondary
        border.color: control.activeFocus ? Theme.AstraTheme.accentAI : Theme.AstraTheme.surfaceElevated
        border.width: 1
    }
}
