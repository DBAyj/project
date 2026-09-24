import QtQuick
import QtQuick.Controls
import "../theme" as Theme

Item {
    objectName: "projectionEmptyState"
    property var controller
    visible: !controller.projectionModel.modelVisible && !Boolean(controller.spatialUIAvailable)
    Accessible.name: "投影状态"
    Accessible.description: controller.lastErrorCode > 0 ? "投影内容已安全清除" : "投影等待中"
    Accessible.role: Accessible.StaticText
    Label {
        anchors.centerIn: parent
        text: controller.lastErrorCode > 0 ? "投影内容已安全清除" : "投影等待中\n请在手机屏幕输入任务"
        horizontalAlignment: Text.AlignHCenter
        color: Theme.AstraTheme.textSecondary
        font.pixelSize: 26
    }
}
