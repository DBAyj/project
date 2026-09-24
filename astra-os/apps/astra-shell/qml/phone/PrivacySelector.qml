import QtQuick
import QtQuick.Controls
import "../theme" as Theme

ComboBox {
    id: selector
    objectName: "privacySelector"
    property alias privacyLevel: selector.currentText
    model: ["PUBLIC", "ROOM_ONLY", "AUTHORIZED_PERSON", "PRIVATE_SCREEN_ONLY", "NO_PROJECTION"]
    Accessible.name: "投影隐私等级"
    Accessible.description: "选择投影内容的隐私等级"
    Accessible.role: Accessible.ComboBox
    contentItem: Label {
        leftPadding: Theme.AstraTheme.spacingSm
        rightPadding: Theme.AstraTheme.spacingSm
        text: selector.displayText
        color: Theme.AstraTheme.textPrimary
        verticalAlignment: Text.AlignVCenter
        font.pixelSize: 13
    }
    background: Rectangle {
        color: Theme.AstraTheme.backgroundSecondary
        radius: Theme.AstraTheme.radiusSm
        border.color: selector.activeFocus ? Theme.AstraTheme.accentProjection : Theme.AstraTheme.surfaceElevated
        border.width: 1
    }
    popup: Popup {
        y: selector.height - 1
        width: selector.width
        implicitHeight: contentItem.implicitHeight
        padding: 1
        contentItem: ListView {
            clip: true
            implicitHeight: contentHeight
            model: selector.popup.visible ? selector.delegateModel : null
            currentIndex: selector.highlightedIndex
            delegate: ItemDelegate {
                width: selector.width
                text: modelData
                highlighted: selector.highlightedIndex === index
            }
        }
        background: Rectangle { color: Theme.AstraTheme.surfaceElevated; radius: Theme.AstraTheme.radiusSm }
    }
}
