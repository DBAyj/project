import QtQuick
import QtQuick.Controls
import "../theme" as Theme

Button {
    id: control
    property color emphasisColor: Theme.AstraTheme.accentPrimary
    implicitHeight: 38
    implicitWidth: Math.max(88, contentItem.implicitWidth + Theme.AstraTheme.spacingLg)
    Accessible.name: text
    Accessible.description: text + "按钮"
    Accessible.role: Accessible.Button
    contentItem: Label {
        text: control.text
        color: control.enabled ? Theme.AstraTheme.backgroundPrimary : Theme.AstraTheme.textSecondary
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
        font.pixelSize: 14
    }
    background: Rectangle {
        radius: Theme.AstraTheme.radiusSm
        color: control.enabled ? control.emphasisColor : Theme.AstraTheme.surfaceElevated
        opacity: control.down ? 0.76 : 1.0
        Behavior on opacity { NumberAnimation { duration: Theme.AstraTheme.animationFast } }
    }
}
