import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../components" as Components
import "../theme" as Theme

Components.AstraCard {
    id: card
    property var controller
    readonly property var result: controller.intentResult
    implicitHeight: content.implicitHeight + Theme.AstraTheme.spacingLg
    Accessible.name: "AI意图分析"
    Accessible.description: result.intent + "，置信度 " + Math.round(result.confidence * 100) + "%"
    Accessible.role: Accessible.StaticText

    ColumnLayout {
        id: content
        anchors.fill: parent
        spacing: Theme.AstraTheme.spacingSm
        RowLayout {
            Layout.fillWidth: true
            Label { text: "AI 意图分析"; color: Theme.AstraTheme.textPrimary; font.pixelSize: 16; font.bold: true; Layout.fillWidth: true }
            Components.AstraStatusBadge {
                text: card.result.serviceStatus
                badgeColor: card.result.serviceStatus === "ONLINE" ? Theme.AstraTheme.statusSuccess : Theme.AstraTheme.statusWarning
            }
        }
        RowLayout {
            Layout.fillWidth: true
            ColumnLayout {
                Layout.fillWidth: true
                spacing: Theme.AstraTheme.spacingXs
                Label {
                    objectName: "analysisIntent"
                    text: card.result.intent
                    color: Theme.AstraTheme.textPrimary
                    font.pixelSize: 14
                    font.bold: true
                    elide: Text.ElideRight
                    Layout.fillWidth: true
                }
                Label {
                    objectName: "analysisConfidence"
                    text: "置信度 " + Math.round(card.result.confidence * 100) + "% · " + card.result.durationMs.toFixed(1) + " ms"
                    color: Theme.AstraTheme.textSecondary
                    font.pixelSize: 12
                }
            }
            Components.AstraStatusBadge {
                objectName: "analysisPolicy"
                text: card.result.executionPolicy
                badgeColor: card.result.executionPolicy === "AUTO_EXECUTE" ? Theme.AstraTheme.statusSuccess
                            : card.result.executionPolicy === "REJECT" ? Theme.AstraTheme.statusError
                            : Theme.AstraTheme.statusWarning
            }
        }
        Label {
            objectName: "analysisSlots"
            Layout.fillWidth: true
            text: "槽位: " + JSON.stringify(card.result.slots)
            color: Theme.AstraTheme.textSecondary
            font.pixelSize: 12
            wrapMode: Text.Wrap
        }
        Label {
            Layout.fillWidth: true
            text: "候选: " + card.result.candidates.map(function(candidate) {
                return candidate.intent + " " + Math.round(candidate.confidence * 100) + "%"
            }).join(" · ")
            color: Theme.AstraTheme.textSecondary
            font.pixelSize: 12
            wrapMode: Text.Wrap
            visible: card.result.candidates.length > 0
        }
        Label {
            objectName: "analysisEngines"
            Layout.fillWidth: true
            text: "规则 " + card.result.ruleEngineStatus + " · 本地模型 " + card.result.localModelStatus
                  + (card.result.fallbackUsed ? " · 已降级" : "") + (card.result.cacheHit ? " · 缓存命中" : "")
            color: card.result.fallbackUsed ? Theme.AstraTheme.statusWarning : Theme.AstraTheme.textSecondary
            font.pixelSize: 12
            wrapMode: Text.Wrap
        }
        Label {
            Layout.fillWidth: true
            text: card.result.clarificationQuestion
            color: Theme.AstraTheme.statusWarning
            font.pixelSize: 13
            wrapMode: Text.Wrap
            visible: card.result.ambiguous && text.length > 0
        }
    }
}
