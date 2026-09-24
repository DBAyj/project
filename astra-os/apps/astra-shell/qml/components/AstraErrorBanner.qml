import QtQuick
import QtQuick.Controls
import "../theme" as Theme

Rectangle {
    id: banner
    property string message: ""
    visible: message.length > 0
    Accessible.name: "错误提示"
    Accessible.description: message
    Accessible.role: Accessible.StaticText
    implicitHeight: visible ? Math.max(34, label.implicitHeight + Theme.AstraTheme.spacingSm) : 0
    color: Qt.rgba(Theme.AstraTheme.statusError.r, Theme.AstraTheme.statusError.g, Theme.AstraTheme.statusError.b, 0.15)
    radius: Theme.AstraTheme.radiusSm
    border.color: Theme.AstraTheme.statusError
    border.width: 1
    Label {
        id: label
        anchors.fill: parent
        anchors.margins: Theme.AstraTheme.spacingSm
        text: banner.message
        color: Theme.AstraTheme.statusError
        wrapMode: Text.Wrap
        verticalAlignment: Text.AlignVCenter
        font.pixelSize: 13
    }
}
