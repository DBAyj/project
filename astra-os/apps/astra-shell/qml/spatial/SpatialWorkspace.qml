import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Item {
    id: workspace
    objectName: displayTarget === "PHONE" ? "phoneSpatialWorkspace" : "projectionSpatialWorkspace"
    property var controller
    property string displayTarget: "PHONE"
    property bool developmentMode: false
    property var spatialModel: controller ? controller.spatialUIModel : null
    property var runtimeComponents: []
    property bool reduceMotion: false
    property bool highContrast: false
    readonly property int renderedComponentCount: countComponentTree(runtimeComponents)
    readonly property int animationDuration: reduceMotion ? 0 : AstraSpatialTheme.animationDuration
    readonly property color workspaceBackground: highContrast ? "#000000" : AstraSpatialTheme.background
    readonly property color workspaceFocusOutline: highContrast ? "#ffdd00" : AstraSpatialTheme.focusOutline

    onReduceMotionChanged: AstraSpatialTheme.reduceMotion = reduceMotion
    onHighContrastChanged: AstraSpatialTheme.highContrast = highContrast
    function syncAccessibilityPreferences() {
        reduceMotion = spatialModel ? Boolean(spatialModel.reduceMotion) : false
        highContrast = spatialModel ? Boolean(spatialModel.highContrast) : false
    }
    function syncRuntimeComponents() {
        runtimeComponents = spatialModel ? spatialModel.componentsForDisplay(displayTarget) : []
    }
    function countComponentTree(components) {
        let count = 0
        for (const component of components || [])
            count += 1 + countComponentTree(component.child_components || [])
        return count
    }
    onSpatialModelChanged: {
        syncAccessibilityPreferences()
        syncRuntimeComponents()
    }
    onDisplayTargetChanged: syncRuntimeComponents()
    Connections {
        target: workspace.spatialModel
        function onChanged() {
            workspace.syncAccessibilityPreferences()
            workspace.syncRuntimeComponents()
        }
    }
    Component.onCompleted: {
        syncAccessibilityPreferences()
        syncRuntimeComponents()
        AstraSpatialTheme.reduceMotion = reduceMotion
        AstraSpatialTheme.highContrast = highContrast
    }

    implicitHeight: displayTarget === "PHONE" ? 330 : 480
    Accessible.name: displayTarget === "PHONE" ? "手机空间工作区" : "投影空间工作区"
    Accessible.description: spatialModel ? "空间组件 " + spatialModel.componentCount : "空间服务离线"
    Accessible.role: Accessible.Pane

    Rectangle {
        anchors.fill: parent
        color: workspace.workspaceBackground
        clip: true

        RowLayout {
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.top: parent.top
            anchors.margins: 12
            Label {
                text: workspace.displayTarget === "PHONE" ? "空间工作区" : "投影空间"
                color: AstraSpatialTheme.text
                font.pixelSize: 16
                font.weight: Font.DemiBold
                Layout.fillWidth: true
            }
            Label {
                text: workspace.spatialModel ? workspace.spatialModel.status : "OFFLINE"
                color: workspace.spatialModel && workspace.spatialModel.status === "READY" ? AstraSpatialTheme.success : AstraSpatialTheme.warning
                font.pixelSize: 11
            }
        }

        Repeater {
            id: componentRepeater
            model: workspace.runtimeComponents
            delegate: SpatialComponentView {
                required property var modelData
                componentData: modelData
                spatialModel: workspace.spatialModel
                controller: workspace.controller
                developmentMode: workspace.developmentMode
                inputCoordinateSystem: workspace.displayTarget === "PHONE" ? "PHONE_VIEW" : "PROJECTION_VIEW"
            }
        }
    }
}
