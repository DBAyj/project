import QtQuick
import QtQml.Models
import QtTest
import "../../qml/components" as Components
import "../../qml/phone" as Phone
import "../../qml/projection" as Projection

TestCase {
    id: testRoot
    property var controllerObject: mockController
    name: "P1Windows"
    when: windowShown

    QtObject {
        id: systemState
        property string systemVersion: "0.1.0-alpha.1"
        property string runtimePlatform: "macOS development host"
        property string cpuArchitecture: "arm64"
        property string qtVersion: "6.11.1"
        property string projectionStatus: "IDLE"
        property string auditStatus: "READY"
        property string configurationStatus: "VALID"
        property int configurationWarningCount: 0
        property var configurationWarnings: []
        property bool usingFallbackConfiguration: false
    }
    QtObject {
        id: projectionState
        property string state: "IDLE"
        property string sessionId: ""
        property string privacyLevel: ""
        property string targetSpace: "desk"
        property bool modelVisible: false
        property bool rotationPaused: false
        property real sceneRotation: 0
        property real zoom: 1
    }
    ListModel { id: taskModel }
    QtObject {
        id: mockIntentResult
        property string serviceStatus: "IDLE"
        property string intent: "unknown"
        property real confidence: 0
        property string executionPolicy: "REJECT"
        property bool requiresConfirmation: false
        property bool ambiguous: false
        property var candidates: []
        property var slots: ({})
        property string ruleEngineStatus: "IDLE"
        property string localModelStatus: "IDLE"
        property bool fallbackUsed: false
        property bool cacheHit: false
        property real durationMs: 0
        property string clarificationQuestion: ""
        property var clarificationOptions: []
        property string confirmationId: ""
        property string confirmationMessage: ""
        property string errorMessage: ""
    }
    QtObject {
        id: mockController
        property string projectionState: projectionState.state
        property string taskResult: "unknown"
        property string errorMessage: ""
        property int lastErrorCode: 0
        property string currentRawText: ""
        property string currentIntent: "unknown"
        property string currentTargetSpace: "desk"
        property string currentPrivacyLevel: "PUBLIC"
        property string currentTaskStatus: "IDLE"
        property string currentRequestId: "request-1"
        property bool projectionFullscreen: false
        property bool p4ProjectionActive: false
        property string p4FramePath: ""
        property bool spatialUIAvailable: false
        property var spatialUIModel: null
        property var systemState: systemState
        property var recentTasks: taskModel
        property var projectionModel: projectionState
        property var intentResult: mockIntentResult
        function submit(text, privacy) { currentRawText = text; currentPrivacyLevel = privacy }
        function stopProjection() { projectionState.state = "IDLE"; projectionState.modelVisible = false }
        function pauseRotation() { projectionState.state = "PAUSED"; projectionState.rotationPaused = true }
        function resumeRotation() { projectionState.state = "ACTIVE"; projectionState.rotationPaused = false }
        function resetView() { projectionState.zoom = 1; projectionState.sceneRotation = 0 }
        function changeZoom(delta) { projectionState.zoom += delta }
        function enterFullscreen() { projectionFullscreen = true }
        function exitFullscreen() { projectionFullscreen = false }
        function rotateView(delta) { projectionState.sceneRotation += delta }
        function confirmIntent() {}
        function rejectIntent() {}
    }

    Component { id: phoneWindowComponent; Phone.PhoneWindow { controller: testRoot.controllerObject } }
    Component { id: projectionWindowComponent; Projection.ProjectionWindow { controller: testRoot.controllerObject } }

    function test_windows_create_and_enforce_minimum_sizes() {
        const phone = phoneWindowComponent.createObject(null)
        const projection = projectionWindowComponent.createObject(null)
        verify(phone !== null)
        verify(projection !== null)
        compare(phone.title, "AstraOS Phone Display")
        compare(phone.minimumWidth, 380)
        compare(phone.minimumHeight, 720)
        compare(projection.title, "AstraOS Projection Display")
        compare(projection.minimumWidth, 800)
        compare(projection.minimumHeight, 450)
        phone.destroy()
        projection.destroy()
    }

    function test_phone_input_and_privacy_controls_are_present() {
        const phone = phoneWindowComponent.createObject(null)
        const selector = findChild(phone, "privacySelector")
        const input = findChild(phone, "taskInput")
        const execute = findChild(phone, "executeButton")
        verify(selector !== null)
        verify(input !== null)
        verify(execute !== null)
        compare(selector.model.length, 5)
        compare(selector.model[0], "PUBLIC")
        compare(selector.model[4], "NO_PROJECTION")
        input.text = "把设备模型投到桌面上"
        execute.clicked()
        compare(mockController.currentRawText, "把设备模型投到桌面上")
        phone.destroy()
    }

    function test_projection_safe_empty_state_and_active_scene() {
        const projection = projectionWindowComponent.createObject(null)
        const emptyState = findChild(projection, "projectionEmptyState")
        const scene = findChild(projection, "projectionScene")
        verify(emptyState.visible)
        verify(!scene.visible)
        projectionState.state = "ACTIVE"
        projectionState.modelVisible = true
        wait(0)
        verify(!emptyState.visible)
        verify(scene.visible)
        projectionState.state = "IDLE"
        projectionState.modelVisible = false
        projection.destroy()
    }

    function test_visible_windows_accept_pointer_interaction() {
        const phone = phoneWindowComponent.createObject(null)
        const projection = projectionWindowComponent.createObject(null)
        verify(phone.visible)
        verify(projection.visible)
        const input = findChild(phone, "taskInput")
        const execute = findChild(phone, "executeButton")
        input.text = "把设备模型投到桌面上"
        mouseClick(execute)
        compare(mockController.currentRawText, "把设备模型投到桌面上")

        projectionState.state = "ACTIVE"
        projectionState.modelVisible = true
        const pause = findChild(projection, "pauseRotationButton")
        const reset = findChild(projection, "resetViewButton")
        const fullscreen = findChild(projection, "fullscreenButton")
        const stop = findChild(projection, "stopProjectionButton")
        verify(pause !== null)
        verify(reset !== null)
        verify(fullscreen !== null)
        verify(stop !== null)
        mouseClick(pause)
        compare(projectionState.state, "PAUSED")
        mouseClick(pause)
        compare(projectionState.state, "ACTIVE")
        projectionState.zoom = 1.4
        projectionState.sceneRotation = 45
        mouseClick(reset)
        compare(projectionState.zoom, 1)
        compare(projectionState.sceneRotation, 0)
        mouseClick(fullscreen)
        verify(mockController.projectionFullscreen)
        mouseClick(fullscreen)
        verify(!mockController.projectionFullscreen)
        mouseClick(stop)
        compare(projectionState.state, "IDLE")
        phone.destroy()
        projection.destroy()
    }

    function test_error_banner_and_privacy_badge_render_chinese_labels() {
        const banner = Qt.createQmlObject('import QtQuick; import "../../qml/components" as Components; Components.AstraErrorBanner { message: "无法识别任务" }', this)
        const badge = Qt.createQmlObject('import QtQuick; import "../../qml/components" as Components; Components.AstraPrivacyBadge { privacyLevel: "ROOM_ONLY" }', this)
        compare(banner.message, "无法识别任务")
        compare(banner.Accessible.name, "错误提示")
        compare(banner.Accessible.description, "无法识别任务")
        compare(badge.text, "仅当前房间可见")
        compare(badge.Accessible.name, "仅当前房间可见")
        banner.destroy()
        badge.destroy()
    }

    function test_interactive_controls_have_accessible_metadata() {
        const phone = phoneWindowComponent.createObject(null)
        const projection = projectionWindowComponent.createObject(null)
        const input = findChild(phone, "taskInput")
        const selector = findChild(phone, "privacySelector")
        const execute = findChild(phone, "executeButton")
        const stop = findChild(projection, "stopProjectionButton")
        const pause = findChild(projection, "pauseRotationButton")
        const reset = findChild(projection, "resetViewButton")
        const fullscreen = findChild(projection, "fullscreenButton")
        verify(input.Accessible.name.length > 0)
        verify(input.Accessible.description.length > 0)
        verify(selector.Accessible.name.length > 0)
        verify(selector.Accessible.description.length > 0)
        verify(execute.Accessible.name.length > 0)
        verify(stop.Accessible.name.length > 0)
        verify(pause.Accessible.name.length > 0)
        verify(reset.Accessible.name.length > 0)
        verify(fullscreen.Accessible.name.length > 0)
        phone.destroy()
        projection.destroy()
    }

    function test_configuration_fallback_warning_is_visible_in_phone_status() {
        const phone = phoneWindowComponent.createObject(null)
        const warning = findChild(phone, "configurationWarningLabel")
        verify(warning !== null)
        verify(!warning.visible)
        systemState.configurationStatus = "INVALID_FALLBACK_ACTIVE"
        systemState.configurationWarnings = ["配置无效，正在使用安全默认值"]
        systemState.configurationWarningCount = 1
        systemState.usingFallbackConfiguration = true
        wait(0)
        verify(warning.visible)
        compare(warning.text, "配置无效，正在使用安全默认值")
        systemState.configurationStatus = "VALID"
        systemState.configurationWarnings = []
        systemState.configurationWarningCount = 0
        systemState.usingFallbackConfiguration = false
        phone.destroy()
    }
}
