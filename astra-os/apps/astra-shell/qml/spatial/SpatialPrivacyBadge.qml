import QtQuick
import QtQuick.Controls

Rectangle {
    id: badge
    property string privacyLevel: "PUBLIC"
    implicitWidth: label.implicitWidth + 18
    implicitHeight: 26
    radius: 4
    color: privacyLevel === "PUBLIC" ? AstraSpatialTheme.projectionAccent : privacyLevel === "ROOM_ONLY" ? AstraSpatialTheme.warning : AstraSpatialTheme.privacy
    Accessible.name: "隐私等级 " + privacyLevel
    Accessible.description: privacyLevel === "PUBLIC" ? "允许公开显示" : "受限内容"
    Accessible.role: Accessible.StaticText
    Label { id: label; anchors.centerIn: parent; text: badge.privacyLevel; color: AstraSpatialTheme.background; font.pixelSize: 10; font.weight: Font.DemiBold }
}
