import QtQuick
import QtQuick.Window
import QtQml
import QtTest
import "../../qml/phone" as Phone

TestCase {
    id: root
    name: "P3SpatialUI"
    when: windowShown

    property int startCalls: 0
    property int detectCalls: 0
    property int selectCalls: 0
    property int calibrateCalls: 0
    property int resetCalls: 0

    QtObject {
        id: spatial
        property string state: "TRACKING"
        property string inputSource: "SIMULATION"
        property string targetState: "CALIBRATED"
        property string quality: "GOOD"
        property string warning: ""
    }

    QtObject {
        id: controller
        property var spatialState: spatial
        function startSpatial() { root.startCalls += 1 }
        function detectSpatial() { root.detectCalls += 1 }
        function selectSpatial() { root.selectCalls += 1 }
        function calibrateSpatial() { root.calibrateCalls += 1 }
        function resetSpatial() { root.resetCalls += 1 }
    }

    Component {
        id: panelComponent
        Window {
            id: host
            width: 430
            height: 340
            visible: true
            property var rootController: controller
            Phone.SpatialStatusPanel { anchors.fill: parent; controller: host.rootController }
        }
    }

    function init() {
        startCalls = 0
        detectCalls = 0
        selectCalls = 0
        calibrateCalls = 0
        resetCalls = 0
    }

    function test_status_and_controls_are_available() {
        const window = panelComponent.createObject(root)
        wait(0)
        const start = findChild(window, "spatialStartButton")
        const detect = findChild(window, "spatialDetectButton")
        const select = findChild(window, "spatialSelectButton")
        const calibrate = findChild(window, "spatialCalibrateButton")
        const reset = findChild(window, "spatialResetButton")
        compare(findChild(window, "spatialStateBadge").text, "TRACKING")
        compare(findChild(window, "spatialTargetBadge").text, "CALIBRATED")
        verify(start.Accessible.name.length > 0)
        start.clicked()
        detect.clicked()
        select.clicked()
        calibrate.clicked()
        reset.clicked()
        compare(startCalls, 1)
        compare(detectCalls, 1)
        compare(selectCalls, 1)
        compare(calibrateCalls, 1)
        compare(resetCalls, 1)
        window.destroy()
    }
}
