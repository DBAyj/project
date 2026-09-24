import QtQuick
import QtQuick.Controls

Menu {
    id: contextMenu
    property bool systemActionsEnabled: false
    MenuItem { text: "聚焦"; Accessible.name: text }
    MenuItem { text: "恢复位置"; Accessible.name: text }
    MenuSeparator {}
    MenuItem { text: "关闭"; enabled: contextMenu.systemActionsEnabled; Accessible.name: text }
}
