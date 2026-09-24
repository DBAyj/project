import QtQuick
import QtQuick.Layouts
import "../components" as Components
import "../theme" as Theme

Rectangle {
    property var controller
    implicitHeight: 58
    color: Theme.AstraTheme.backgroundSecondary
    RowLayout {
        anchors.centerIn: parent; spacing: Theme.AstraTheme.spacingSm
        Components.AstraButton {
            objectName: "stopProjectionButton"
            text: "停止投影"
            emphasisColor: Theme.AstraTheme.statusError
            onClicked: controller.stopProjection()
        }
        Components.AstraButton {
            objectName: "resetViewButton"
            text: "重置视角"
            emphasisColor: Theme.AstraTheme.surfaceElevated
            onClicked: controller.resetView()
        }
        Components.AstraButton {
            objectName: "pauseRotationButton"
            text: controller.projectionModel.rotationPaused ? "继续旋转" : "暂停旋转"
            emphasisColor: Theme.AstraTheme.surfaceElevated
            onClicked: controller.projectionModel.rotationPaused ? controller.resumeRotation() : controller.pauseRotation()
        }
        Components.AstraButton {
            objectName: "fullscreenButton"
            text: controller.projectionFullscreen ? "退出全屏" : "全屏"
            emphasisColor: Theme.AstraTheme.accentPrimary
            onClicked: controller.projectionFullscreen ? controller.exitFullscreen() : controller.enterFullscreen()
        }
    }
}
