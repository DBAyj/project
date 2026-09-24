pragma Singleton
import QtQuick
import "../theme" as Base

QtObject {
    property bool reduceMotion: false
    property bool highContrast: false
    readonly property color background: highContrast ? "#000000" : Base.AstraTheme.backgroundPrimary
    readonly property color surface: highContrast ? "#111111" : Base.AstraTheme.surfacePrimary
    readonly property color surfaceRaised: highContrast ? "#242424" : Base.AstraTheme.surfaceElevated
    readonly property color text: highContrast ? "#ffffff" : Base.AstraTheme.textPrimary
    readonly property color textMuted: highContrast ? "#f0f0f0" : Base.AstraTheme.textSecondary
    readonly property color accent: highContrast ? "#00e5ff" : Base.AstraTheme.accentPrimary
    readonly property color aiAccent: Base.AstraTheme.accentAI
    readonly property color projectionAccent: Base.AstraTheme.accentProjection
    readonly property color privacy: Base.AstraTheme.privacyPrivate
    readonly property color success: Base.AstraTheme.statusSuccess
    readonly property color warning: Base.AstraTheme.statusWarning
    readonly property color error: Base.AstraTheme.statusError
    readonly property color focusOutline: highContrast ? "#ffdd00" : "#ffffff"
    readonly property int radius: Base.AstraTheme.radiusMd
    readonly property int spacing: Base.AstraTheme.spacingMd
    readonly property int animationDuration: reduceMotion ? 0 : Base.AstraTheme.animationNormal
}
