import QtQuick
import QtQuick.Controls
import "../theme" as Theme

Rectangle {
    id: badge
    property string text: ""
    property color badgeColor: Theme.AstraTheme.statusSuccess
    Accessible.name: text
    Accessible.description: "系统状态：" + text
    Accessible.role: Accessible.StaticText
    implicitWidth: label.implicitWidth + Theme.AstraTheme.spacingMd
    implicitHeight: 24
    radius: Theme.AstraTheme.radiusSm
    color: Qt.rgba(badgeColor.r, badgeColor.g, badgeColor.b, 0.18)
    border.color: badgeColor
    border.width: 1
    Label {
        id: label
        anchors.centerIn: parent
        text: badge.text
        color: badge.badgeColor
        font.pixelSize: 12
    }
}
