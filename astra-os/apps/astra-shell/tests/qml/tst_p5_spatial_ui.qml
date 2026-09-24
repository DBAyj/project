import QtQuick
import QtQuick.Controls
import QtTest
import "../../qml/components" as Components
import "../../qml/spatial" as Spatial

TestCase {
    id: root
    name: "P5SpatialUI"
    when: windowShown

    QtObject {
        id: spatialModelFixture
        property string status: "READY"
        property int componentCount: 5
        property int windowCount: 1
        property string focusComponentId: "7abf7b1e-dc4f-4b7c-96cc-c3dd628e1ca9"
        property bool projectionSafe: false
        property bool reduceMotion: false
        property bool highContrast: false
        property string p4ReleaseStatus: "P4_RELEASE_BASELINE_FINAL"
        property var components: []
        function componentsForDisplay(displayTarget) { return components }
        signal changed()
    }
    QtObject {
        id: testController
        property var spatialUIModel: spatialModelFixture
        property bool spatialUIAvailable: true
        property string currentRawText: "检查设备模型"
        property string taskResult: "open_task_surface"
        property string currentTaskStatus: "READY"
        property string currentPrivacyLevel: "PUBLIC"
        property string activatedComponentId: ""
        property string activatedAction: ""
        property string routedComponentId: ""
        property string routedEventType: ""
        property string routedSourceType: ""
        function activateSpatialComponent(componentId, action) {
            activatedComponentId = componentId
            activatedAction = action
        }
        function routeSpatialInput(componentId, eventType, sourceType, coordinateSystem, x, y) {
            routedComponentId = componentId
            routedEventType = eventType
            routedSourceType = sourceType
        }
    }

    Component { id: workspaceComponent; Spatial.SpatialWorkspace { width: 900; height: 520; controller: testController } }
    Component { id: windowComponent; Spatial.SpatialWindowView { windowId: "d13d0ba9-8de1-4c38-97ae-09f9d56e9935" } }
    Component { id: taskComponent; Spatial.SpatialTaskCard { taskTitle: "任务"; summary: "摘要" } }
    Component { id: systemComponent; Spatial.SpatialSystemPanel { spatialModel: spatialModelFixture } }
    Component { id: notificationComponent; Spatial.SpatialNotificationView { title: "通知"; message: "内容" } }
    Component { id: privacyComponent; Spatial.SpatialPrivacyBadge { privacyLevel: "PRIVATE_SCREEN_ONLY" } }
    Component { id: contextMenuComponent; Spatial.SpatialContextMenu {} }
    Component { id: debugComponent; Spatial.SpatialDebugOverlay {} }

    function test_spatial_components_create() {
        const values = [workspaceComponent.createObject(root), windowComponent.createObject(root), taskComponent.createObject(root),
                        systemComponent.createObject(root), notificationComponent.createObject(root), privacyComponent.createObject(root),
                        contextMenuComponent.createObject(root), debugComponent.createObject(root)]
        for (const value of values) verify(value !== null)
        compare(values[0].Accessible.name, "手机空间工作区")
        compare(values[2].Accessible.name, "任务")
        compare(values[4].Accessible.name, "通知")
        verify(!values[7].visible)
        for (const value of values) value.destroy()
    }

    function test_debug_overlay_only_in_development_mode() {
        const overlay = debugComponent.createObject(null)
        verify(!overlay.visible)
        overlay.developmentMode = true
        verify(overlay.developmentMode)
        overlay.destroy()
    }

    function test_spatial_controls_are_keyboard_focusable_and_named() {
        const button = Qt.createQmlObject('import QtQuick; import "../../qml/components" as Components; Components.AstraSpatialButton { text: "确认" }', root)
        const slider = Qt.createQmlObject('import QtQuick; import "../../qml/components" as Components; Components.AstraSpatialSlider { objectName: "缩放" }', root)
        const toggle = Qt.createQmlObject('import QtQuick; import "../../qml/components" as Components; Components.AstraSpatialToggle { text: "减少动态效果" }', root)
        verify(button.activeFocusOnTab && button.Accessible.name.length > 0)
        verify(slider.activeFocusOnTab && slider.Accessible.name.length > 0)
        verify(toggle.activeFocusOnTab && toggle.Accessible.name.length > 0)
        button.destroy()
        slider.destroy()
        toggle.destroy()
    }

    function test_accessibility_preferences_change_runtime_theme() {
        const workspace = workspaceComponent.createObject(root)
        verify(workspace !== null)
        compare(workspace.spatialModel, spatialModelFixture)
        spatialModelFixture.reduceMotion = true
        spatialModelFixture.highContrast = true
        spatialModelFixture.changed()
        tryCompare(workspace, "animationDuration", 0)
        compare(workspace.workspaceBackground.toString(), "#000000")
        compare(workspace.workspaceFocusOutline.toString(), "#ffdd00")
        spatialModelFixture.reduceMotion = false
        spatialModelFixture.highContrast = false
        spatialModelFixture.changed()
        workspace.destroy()
    }

    function test_workspace_renders_runtime_component_tree() {
        spatialModelFixture.components = [
            { component_id: "runtime-task", component_type: "TASK_CARD", display_target: "PHONE", privacy_level: "PUBLIC",
              accessibility_label: "运行时任务", content: { title: "真实任务", summary: "真实摘要", state: "WAITING_CONFIRMATION",
                progress: 0.75, intent_type: "project_model", confidence: 0.91, execution_strategy: "REQUIRE_CONFIRMATION" },
              bounds: { x: 24, y: 72, width: 360, height: 160 }, visible: true },
            { component_id: "runtime-notification", component_type: "NOTIFICATION", display_target: "PHONE", privacy_level: "PUBLIC",
              accessibility_label: "运行时通知", content: { title: "安全确认", message: "是否继续", severity: "CRITICAL", actions: ["确认"] },
              bounds: { x: 420, y: 72, width: 320, height: 160 }, visible: true },
            { component_id: "runtime-toggle", component_type: "TOGGLE", display_target: "PHONE", privacy_level: "PUBLIC",
              accessibility_label: "运行时开关", bounds: { x: 24, y: 250, width: 180, height: 48 }, visible: true },
            { component_id: "runtime-slider", component_type: "SLIDER", display_target: "PHONE", privacy_level: "PUBLIC",
              accessibility_label: "运行时滑块", bounds: { x: 220, y: 250, width: 220, height: 48 }, visible: true },
            { component_id: "runtime-window", component_type: "SPATIAL_WINDOW", display_target: "PHONE", privacy_level: "PUBLIC",
              accessibility_label: "运行时窗口", bounds: { x: 460, y: 250, width: 260, height: 120 }, visible: true }
        ]
        const workspace = workspaceComponent.createObject(root)
        verify(workspace !== null)
        tryCompare(workspace, "renderedComponentCount", 5)
        const taskView = findChild(workspace, "runtimeComponent-runtime-task")
        verify(taskView !== null && taskView.item !== null)
        compare(taskView.item.intentType, "project_model")
        compare(taskView.item.executionStrategy, "REQUIRE_CONFIRMATION")
        const confirmButton = findChild(taskView.item, "taskConfirmButton")
        verify(confirmButton !== null)
        tryCompare(confirmButton, "enabled", true)
        testController.routedComponentId = ""
        testController.routedEventType = ""
        testController.routedSourceType = ""
        confirmButton.forceActiveFocus(Qt.TabFocusReason)
        tryCompare(testController, "routedComponentId", "runtime-task")
        compare(testController.routedEventType, "SYSTEM_FOCUS")
        compare(testController.routedSourceType, "KEYBOARD")
        const cancelButton = findChild(taskView.item, "taskCancelButton")
        verify(cancelButton !== null)
        compare(confirmButton.KeyNavigation.tab, cancelButton)
        compare(cancelButton.KeyNavigation.backtab, confirmButton)
        confirmButton.clicked()
        compare(testController.activatedComponentId, "runtime-task")
        compare(testController.activatedAction, "confirm")
        const notificationView = findChild(workspace, "runtimeComponent-runtime-notification")
        verify(notificationView !== null)
        const notificationItem = notificationView.item || notificationView
        tryCompare(notificationItem, "actionCount", 1)
        notificationItem.triggerAction(0)
        compare(testController.activatedComponentId, "runtime-notification")
        compare(testController.activatedAction, "确认")

        const toggleView = findChild(workspace, "runtimeComponent-runtime-toggle")
        verify(toggleView !== null && toggleView.item !== null)
        testController.routedComponentId = ""
        toggleView.item.forceActiveFocus(Qt.TabFocusReason)
        tryCompare(testController, "routedComponentId", "runtime-toggle")
        compare(testController.routedEventType, "SYSTEM_FOCUS")
        compare(testController.routedSourceType, "KEYBOARD")
        toggleView.item.toggled()
        compare(testController.routedComponentId, "runtime-toggle")
        compare(testController.routedEventType, "POINTER_RELEASE")
        compare(testController.routedSourceType, "MOUSE")
        const sliderView = findChild(workspace, "runtimeComponent-runtime-slider")
        verify(sliderView !== null && sliderView.item !== null)
        sliderView.item.moved()
        compare(testController.routedComponentId, "runtime-slider")
        compare(testController.routedEventType, "POINTER_SCROLL")
        const windowView = findChild(workspace, "runtimeComponent-runtime-window")
        verify(windowView !== null && windowView.item !== null)
        windowView.item.inputRequested("SYSTEM_FOCUS", "MOUSE")
        compare(testController.routedComponentId, "runtime-window")
        compare(testController.routedEventType, "SYSTEM_FOCUS")

        spatialModelFixture.components = [spatialModelFixture.components[0]]
        spatialModelFixture.changed()
        tryCompare(workspace, "renderedComponentCount", 1)
        verify(findChild(workspace, "runtimeComponent-runtime-notification") === null)
        workspace.destroy()
        spatialModelFixture.components = []
    }

    function test_workspace_preserves_runtime_parent_child_items() {
        spatialModelFixture.components = [
            { component_id: "runtime-parent", component_type: "SPATIAL_WINDOW", display_target: "PHONE", privacy_level: "PUBLIC",
              accessibility_label: "父窗口", bounds: { x: 40, y: 60, width: 420, height: 260 }, visible: true,
              child_components: [
                  { component_id: "runtime-child", component_type: "BUTTON", display_target: "PHONE", privacy_level: "PUBLIC",
                    accessibility_label: "子按钮", bounds: { x: 24, y: 48, width: 120, height: 44 }, visible: true,
                    hierarchy_depth: 1, child_components: [] }
              ] }
        ]
        const workspace = workspaceComponent.createObject(root)
        verify(workspace !== null)
        tryCompare(workspace, "renderedComponentCount", 2)
        const parentView = findChild(workspace, "runtimeComponent-runtime-parent")
        const childView = findChild(workspace, "runtimeComponent-runtime-child")
        verify(parentView !== null && childView !== null)
        compare(childView.parent.parent, parentView)
        compare(childView.x, 24)
        compare(childView.y, 48)
        workspace.destroy()
        spatialModelFixture.components = []
    }
}
