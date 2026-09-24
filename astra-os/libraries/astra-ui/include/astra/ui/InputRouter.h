#pragma once

#include "astra/ui/Types.h"

#include <QList>
#include <QPointF>
#include <QPolygonF>

#include <optional>

namespace astra::ui {

enum class InputSourceType { Mouse, Keyboard, Touch, SimulatedGesture, System, AiIntent };
enum class InputEventType {
    PointerMove, PointerPress, PointerRelease, PointerScroll, KeyPress, KeyRelease,
    TouchBegin, TouchUpdate, TouchEnd, GestureSelect, GestureGrab, GestureRelease,
    GestureScale, GestureRotate, SystemFocus, AiAction
};
enum class CoordinateSystem { PhoneView, ProjectionView, TargetLocal, AnchorLocal };
enum class HitShape { Rectangle, Circle, Polygon };

struct InputEvent {
    QString eventId;
    InputEventType eventType {InputEventType::PointerMove};
    InputSourceType sourceType {InputSourceType::Mouse};
    QString sourceId;
    CoordinateSystem coordinateSystem {CoordinateSystem::PhoneView};
    double x {0.0};
    double y {0.0};
    QString targetComponentId;
    std::optional<double> interactionValue;
    bool targetCaptured {false};
};

struct HitTarget {
    QString componentId;
    SpatialBounds bounds;
    HitShape shape {HitShape::Rectangle};
    QPolygonF polygon;
    int zOrder {0};
    bool securityLayer {false};
    bool modal {false};
    bool visible {true};
    bool interactive {true};
    double opacity {1.0};
    bool enabled {true};
    QString parentComponentId;
    int hierarchyDepth {0};
};

struct InputRouteResult : OperationResult {
    QString targetComponentId;
    bool blocked {false};
};

class InputRouter final {
public:
    void setTargets(QList<HitTarget> targets);
    [[nodiscard]] InputRouteResult route(const InputEvent &event) const;

private:
    [[nodiscard]] static bool hit(const HitTarget &target, double x, double y);
    QList<HitTarget> targets_;
};

} // namespace astra::ui
