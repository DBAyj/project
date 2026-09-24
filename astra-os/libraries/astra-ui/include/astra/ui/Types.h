#pragma once

#include "astra/common/PrivacyLevel.h"

#include <QString>

namespace astra::ui {

struct SpatialBounds {
    double x {0.0};
    double y {0.0};
    double width {0.0};
    double height {0.0};

    [[nodiscard]] bool isValid() const;
    [[nodiscard]] bool contains(double pointX, double pointY) const;
};

struct SpatialTransform {
    double translationX {0.0};
    double translationY {0.0};
    double rotationDegrees {0.0};
    double scale {1.0};
};

enum class ComponentType {
    SpatialWindow, TaskCard, SystemPanel, Notification, Button, Toggle, Slider, Label, Image,
    ModelView, PrivacyBadge, ContextMenu, DebugPanel, SystemSecurityOverlay, PrivacyMask,
    SystemCriticalAlert
};

enum class ComponentLifecycleState { Created, Attached, Visible, Focused, Interacting, Hidden, Suspended, Detached, Destroyed, Error };
enum class Principal { Application, System };
enum class DisplayTarget { Phone, Projection, Both };

struct OperationResult {
    bool ok {false};
    int errorCode {0};
    QString message;
};

[[nodiscard]] bool isSystemOnly(ComponentType type);

} // namespace astra::ui
