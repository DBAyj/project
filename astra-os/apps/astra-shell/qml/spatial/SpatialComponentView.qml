import QtQuick
import QtQuick.Controls
import "../components" as Components

Loader {
    id: componentView
    required property var componentData
    property var spatialModel
    property var controller
    property bool developmentMode: false
    property string inputCoordinateSystem: "PHONE_VIEW"
    readonly property var componentBounds: componentData && componentData.bounds ? componentData.bounds : ({})
    readonly property string componentType: componentData ? String(componentData.component_type || "") : ""
    readonly property string componentId: componentData ? String(componentData.component_id || "") : ""
    readonly property string accessibilityLabel: componentData ? String(componentData.accessibility_label || componentType) : ""
    readonly property string privacyLevel: componentData ? String(componentData.privacy_level || "PRIVATE_SCREEN_ONLY") : "PRIVATE_SCREEN_ONLY"
    readonly property var contentData: componentData && componentData.content ? componentData.content : ({})
    readonly property var transformData: componentData && componentData.transform ? componentData.transform : ({})
    readonly property int hierarchyDepth: Number(componentData && componentData.hierarchy_depth || 0)
    readonly property bool componentVisible: !componentData || componentData.visible === undefined || Boolean(componentData.visible)
    readonly property var childComponents: componentData && componentData.child_components ? componentData.child_components : []

    function routeUserInput(eventType, sourceType) {
        if (!controller)
            return
        const point = mapToItem(null, width / 2, height / 2)
        controller.routeSpatialInput(componentId, eventType, sourceType, inputCoordinateSystem, point.x, point.y)
    }

    objectName: "runtimeComponent-" + componentId
    x: Number(componentBounds.x || 0) + Number(transformData.translation_x || 0)
    y: Number(componentBounds.y || 0) + Number(transformData.translation_y || 0)
    width: Math.max(1, Number(componentBounds.width || 320))
    height: Math.max(1, Number(componentBounds.height || 120))
    z: Number(componentData && componentData.z_order || 0) + hierarchyDepth * 0.001
    rotation: Number(transformData.rotation_degrees || 0)
    scale: Number(transformData.scale || 1)
    opacity: componentData && componentData.opacity !== undefined ? Number(componentData.opacity) : 1
    enabled: !componentData || componentData.enabled === undefined || Boolean(componentData.enabled)
    visible: componentVisible
    active: componentVisible
    sourceComponent: {
        switch (componentType) {
        case "SPATIAL_WINDOW": return windowDelegate
        case "TASK_CARD": return taskDelegate
        case "SYSTEM_PANEL": return systemDelegate
        case "NOTIFICATION": return notificationDelegate
        case "BUTTON": return buttonDelegate
        case "TOGGLE": return toggleDelegate
        case "SLIDER": return sliderDelegate
        case "PRIVACY_BADGE": return privacyDelegate
        case "DEBUG_PANEL": return debugDelegate
        default: return genericDelegate
        }
    }

    Component {
        id: windowDelegate
        SpatialWindowView {
            anchors.fill: parent
            windowId: componentView.componentId
            focused: Boolean(componentView.spatialModel && componentView.spatialModel.focusComponentId === componentView.componentId)
            privacyLevel: componentView.privacyLevel
            onInputRequested: (eventType, sourceType) => {
                componentView.routeUserInput(eventType, sourceType)
            }
        }
    }
    Component {
        id: taskDelegate
        SpatialTaskCard {
            anchors.fill: parent
            taskTitle: String(componentView.contentData.title || componentView.accessibilityLabel)
            summary: String(componentView.contentData.summary || "")
            status: String(componentView.contentData.state || "READY")
            progress: Number(componentView.contentData.progress || 0)
            intentType: String(componentView.contentData.intent_type || "")
            confidence: Number(componentView.contentData.confidence || 0)
            executionStrategy: String(componentView.contentData.execution_strategy || "")
            privacyLevel: componentView.privacyLevel
            onConfirmRequested: {
                if (componentView.controller) componentView.controller.activateSpatialComponent(componentView.componentId, "confirm")
            }
            onCancelRequested: {
                if (componentView.controller) componentView.controller.activateSpatialComponent(componentView.componentId, "cancel")
            }
            onFocusRequested: componentView.routeUserInput("SYSTEM_FOCUS", "KEYBOARD")
        }
    }
    Component {
        id: systemDelegate
        SpatialSystemPanel { anchors.fill: parent; spatialModel: componentView.spatialModel }
    }
    Component {
        id: notificationDelegate
        SpatialNotificationView {
            anchors.fill: parent
            title: String(componentView.contentData.title || componentView.accessibilityLabel)
            message: String(componentView.contentData.message || "")
            severity: String(componentView.contentData.severity || "INFO")
            actions: componentView.contentData.actions || []
            onActionTriggered: action => {
                if (componentView.controller) componentView.controller.activateSpatialComponent(componentView.componentId, action)
            }
            onFocusRequested: componentView.routeUserInput("SYSTEM_FOCUS", "KEYBOARD")
        }
    }
    Component {
        id: buttonDelegate
        Components.AstraSpatialButton {
            anchors.fill: parent
            text: componentView.accessibilityLabel
            onActiveFocusChanged: {
                if (activeFocus) componentView.routeUserInput("SYSTEM_FOCUS", "KEYBOARD")
            }
            onClicked: componentView.routeUserInput("POINTER_RELEASE", "MOUSE")
        }
    }
    Component {
        id: toggleDelegate
        Components.AstraSpatialToggle {
            anchors.fill: parent
            text: componentView.accessibilityLabel
            onActiveFocusChanged: {
                if (activeFocus) componentView.routeUserInput("SYSTEM_FOCUS", "KEYBOARD")
            }
            onToggled: componentView.routeUserInput("POINTER_RELEASE", "MOUSE")
        }
    }
    Component {
        id: sliderDelegate
        Components.AstraSpatialSlider {
            anchors.fill: parent
            objectName: componentView.accessibilityLabel
            onActiveFocusChanged: {
                if (activeFocus) componentView.routeUserInput("SYSTEM_FOCUS", "KEYBOARD")
            }
            onMoved: componentView.routeUserInput("POINTER_SCROLL", "MOUSE")
        }
    }
    Component {
        id: privacyDelegate
        SpatialPrivacyBadge { anchors.centerIn: parent; privacyLevel: componentView.privacyLevel }
    }
    Component {
        id: debugDelegate
        SpatialDebugOverlay {
            anchors.fill: parent
            developmentMode: componentView.developmentMode
            focusOwner: componentView.spatialModel ? componentView.spatialModel.focusComponentId : "none"
        }
    }
    Component {
        id: genericDelegate
        Components.AstraSpatialCard {
            anchors.fill: parent
            Accessible.name: componentView.accessibilityLabel
            Accessible.role: Accessible.StaticText
            Label {
                anchors.fill: parent
                anchors.margins: 12
                text: componentView.accessibilityLabel
                color: AstraSpatialTheme.text
                wrapMode: Text.Wrap
                verticalAlignment: Text.AlignVCenter
            }
        }
    }

    Repeater {
        model: componentView.childComponents
        delegate: Loader {
            id: childLoader
            required property var modelData
            Component.onCompleted: {
                setSource("SpatialComponentView.qml", {
                    componentData: modelData,
                    spatialModel: componentView.spatialModel,
                    controller: componentView.controller,
                    developmentMode: componentView.developmentMode,
                    inputCoordinateSystem: componentView.inputCoordinateSystem
                })
            }
        }
    }
}
