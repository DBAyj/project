import QtQuick
import QtQuick.Controls

Rectangle {
    id: overlay
    property bool developmentMode: false
    property string focusOwner: "none"
    visible: developmentMode
    color: "#cc0d1117"
    border.width: 1
    border.color: AstraSpatialTheme.warning
    implicitWidth: 240
    implicitHeight: 76
    Accessible.name: "空间调试信息"
    Accessible.description: "当前焦点 " + focusOwner
    Label { anchors.fill: parent; anchors.margins: 10; text: "DEBUG OVERLAY\nfocus=" + overlay.focusOwner; color: AstraSpatialTheme.warning; font.pixelSize: 11 }
}
