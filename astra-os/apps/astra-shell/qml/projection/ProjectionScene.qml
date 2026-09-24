import QtQuick
import QtQuick3D
import "../theme" as Theme

View3D {
    id: scene
    objectName: "projectionScene"
    property var controller
    visible: controller.projectionModel.modelVisible && !controller.p4ProjectionActive
    environment: SceneEnvironment { clearColor: Theme.AstraTheme.backgroundPrimary; backgroundMode: SceneEnvironment.Color; antialiasingMode: SceneEnvironment.MSAA; antialiasingQuality: SceneEnvironment.High }
    PerspectiveCamera {
        position: Qt.vector3d(controller.projectionModel.cameraOffsetX,
                              controller.projectionModel.cameraOffsetY,
                              550 / controller.projectionModel.zoom + controller.projectionModel.cameraOffsetZ)
    }
    DirectionalLight { eulerRotation.x: -35; eulerRotation.y: -25; brightness: 2.0 }
    PointLight { position: Qt.vector3d(120, 160, 180); brightness: 45; color: Theme.AstraTheme.accentProjection }
    Node {
        id: manualOrientation
        eulerRotation.y: controller.projectionModel.sceneRotation
        Node {
            id: autoOrientation
            NumberAnimation on eulerRotation.y { from: 0; to: 360; duration: 9000; loops: Animation.Infinite; running: scene.visible && !scene.controller.projectionModel.rotationPaused }
            Model { source: "#Cube"; scale: Qt.vector3d(1.45, 0.48, 0.9); materials: PrincipledMaterial { baseColor: Theme.AstraTheme.accentProjection; metalness: 0.36; roughness: 0.24 } }
            Model { source: "#Cylinder"; position: Qt.vector3d(0, 90, 0); scale: Qt.vector3d(0.48, 0.5, 0.48); materials: PrincipledMaterial { baseColor: Theme.AstraTheme.accentPrimary; metalness: 0.25; roughness: 0.3 } }
            Model { source: "#Sphere"; position: Qt.vector3d(0, 155, 0); scale: Qt.vector3d(0.32, 0.32, 0.32); materials: PrincipledMaterial { baseColor: Theme.AstraTheme.accentAI; metalness: 0.2; roughness: 0.18 } }
        }
    }
    MouseArea {
        anchors.fill: parent
        acceptedButtons: Qt.LeftButton
        property real previousX: 0
        onPressed: mouse => previousX = mouse.x
        onPositionChanged: mouse => {
            if (pressed) {
                scene.controller.rotateView(mouse.x - previousX)
                previousX = mouse.x
            }
        }
    }
    WheelHandler { onWheel: event => scene.controller.changeZoom(event.angleDelta.y > 0 ? 0.1 : -0.1) }
}
