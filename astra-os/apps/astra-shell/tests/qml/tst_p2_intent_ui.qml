import QtQuick
import QtQuick.Window
import QtQml
import QtTest
import "../../qml/phone" as Phone

TestCase {
    id: root
    name: "P2IntentUI"
    when: windowShown

    property int confirmationCount: 0
    property int rejectionCount: 0
    property int reentryCount: 0

    QtObject {
        id: mockIntentResult
        property string serviceStatus: "ONLINE"
        property string intent: "project_3d_model"
        property real confidence: 0.97
        property string executionPolicy: "REQUIRE_CONFIRMATION"
        property bool requiresConfirmation: true
        property bool ambiguous: false
        property var candidates: [
            {"intent": "project_3d_model", "confidence": 0.97, "source": "RULE_ENGINE"},
            {"intent": "start_projection", "confidence": 0.71, "source": "LOCAL_MODEL"}
        ]
        property var slots: {"target_space": "desk", "model_id": "demo-device", "privacy_level": "PUBLIC"}
        property string ruleEngineStatus: "READY"
        property string localModelStatus: "READY"
        property bool fallbackUsed: false
        property bool cacheHit: false
        property real durationMs: 8.5
        property string clarificationQuestion: ""
        property var clarificationOptions: []
        property string confirmationId: "confirmation-1"
        property string confirmationMessage: "该操作需要确认，是否继续？"
        property string errorMessage: ""
    }

    QtObject {
        id: mockController
        property var intentResult: mockIntentResult
        function confirmIntent() { root.confirmationCount += 1 }
        function rejectIntent() { root.rejectionCount += 1 }
    }

    Component { id: analysisComponent; Phone.IntentAnalysisCard { controller: mockController } }
    Component {
        id: confirmationComponent
        Window {
            width: 430
            height: 300
            visible: true
            Phone.ConfirmationPanel {
                anchors.fill: parent
                controller: mockController
                onReenterRequested: root.reentryCount += 1
            }
        }
    }

    function init() {
        confirmationCount = 0
        rejectionCount = 0
        reentryCount = 0
        mockIntentResult.requiresConfirmation = true
    }

    function test_analysis_card_presents_protocol_evidence() {
        const card = analysisComponent.createObject(root)
        verify(card !== null)
        compare(findChild(card, "analysisIntent").text, "project_3d_model")
        compare(findChild(card, "analysisPolicy").text, "REQUIRE_CONFIRMATION")
        verify(findChild(card, "analysisConfidence").text.indexOf("97") >= 0)
        verify(findChild(card, "analysisSlots").text.indexOf("desk") >= 0)
        verify(findChild(card, "analysisEngines").text.indexOf("READY") >= 0)
        card.destroy()
    }

    function test_confirmation_controls_are_accessible_and_dispatch_actions() {
        compare(mockIntentResult.requiresConfirmation, true)
        const window = confirmationComponent.createObject(root)
        wait(0)
        const panel = findChild(window, "confirmationPanel")
        compare(panel.result.requiresConfirmation, true)
        verify(panel.visible)
        const confirm = findChild(panel, "confirmIntentButton")
        const reject = findChild(panel, "rejectIntentButton")
        const reenter = findChild(panel, "reenterIntentButton")
        verify(confirm.Accessible.name.length > 0)
        verify(reject.Accessible.name.length > 0)
        verify(reenter.Accessible.name.length > 0)
        confirm.clicked()
        reject.clicked()
        reenter.clicked()
        compare(confirmationCount, 1)
        compare(rejectionCount, 1)
        compare(reentryCount, 1)
        window.destroy()
    }

    function test_confirmation_panel_hides_when_not_required() {
        mockIntentResult.requiresConfirmation = false
        const window = confirmationComponent.createObject(root)
        wait(0)
        const panel = findChild(window, "confirmationPanel")
        verify(!panel.visible)
        window.destroy()
    }
}
