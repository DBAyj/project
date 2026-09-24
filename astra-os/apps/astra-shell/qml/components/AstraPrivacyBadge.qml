import QtQuick
import QtQuick.Controls
import "../theme" as Theme

Rectangle {
    id: badge
    property string privacyLevel: "PUBLIC"
    Accessible.name: text
    Accessible.description: "投影隐私等级：" + text
    Accessible.role: Accessible.StaticText
    readonly property string text: {
        if (privacyLevel === "ROOM_ONLY") return "仅当前房间可见"
        if (privacyLevel === "AUTHORIZED_PERSON") return "已授权人员"
        if (privacyLevel === "PRIVATE_SCREEN_ONLY") return "仅手机屏幕"
        if (privacyLevel === "NO_PROJECTION") return "禁止投影"
        return "公开"
    }
    readonly property color privacyColor: {
        if (privacyLevel === "PUBLIC") return Theme.AstraTheme.privacyPublic
        if (privacyLevel === "ROOM_ONLY" || privacyLevel === "AUTHORIZED_PERSON") return Theme.AstraTheme.privacyRestricted
        return Theme.AstraTheme.privacyPrivate
    }
    implicitWidth: label.implicitWidth + Theme.AstraTheme.spacingMd
    implicitHeight: 24
    radius: Theme.AstraTheme.radiusSm
    color: Qt.rgba(privacyColor.r, privacyColor.g, privacyColor.b, 0.16)
    border.color: privacyColor
    border.width: 1
    Label {
        id: label
        anchors.centerIn: parent
        text: badge.text
        color: badge.privacyColor
        font.pixelSize: 12
    }
}
