#include "astra/ui/InputRouter.h"

#include <algorithm>
#include <cmath>

namespace astra::ui {

void InputRouter::setTargets(QList<HitTarget> targets)
{
    std::stable_sort(targets.begin(), targets.end(), [](const HitTarget &left, const HitTarget &right) {
        if (left.securityLayer != right.securityLayer) return left.securityLayer;
        if (left.modal != right.modal) return left.modal;
        if (left.zOrder != right.zOrder) return left.zOrder > right.zOrder;
        if (left.parentComponentId == right.componentId) return true;
        if (right.parentComponentId == left.componentId) return false;
        if (left.hierarchyDepth != right.hierarchyDepth) return left.hierarchyDepth > right.hierarchyDepth;
        return left.componentId < right.componentId;
    });
    targets_ = std::move(targets);
}

InputRouteResult InputRouter::route(const InputEvent &event) const
{
    if (event.eventId.isEmpty() || event.sourceId.isEmpty() || !std::isfinite(event.x) || !std::isfinite(event.y)) {
        return {{false, 5402, QStringLiteral("Invalid input event")}, {}, false};
    }
    const bool directTargetEvent = event.eventType == InputEventType::KeyPress
        || event.eventType == InputEventType::KeyRelease
        || event.eventType == InputEventType::SystemFocus
        || event.eventType == InputEventType::AiAction;
    const bool targeted = (directTargetEvent || event.targetCaptured) && !event.targetComponentId.isEmpty();
    if (targeted) {
        const auto safetyLayer = std::find_if(targets_.cbegin(), targets_.cend(), [](const HitTarget &target) {
            return target.securityLayer && target.visible && target.interactive && target.enabled && target.opacity > 0.0;
        });
        if (safetyLayer != targets_.cend() && safetyLayer->componentId != event.targetComponentId) {
            return {{false, 5406, QStringLiteral("Input blocked by safety layer")}, safetyLayer->componentId, true};
        }
        const auto iterator = std::find_if(targets_.cbegin(), targets_.cend(), [&event](const HitTarget &target) {
            return target.componentId == event.targetComponentId;
        });
        if (iterator == targets_.cend() || !iterator->visible || !iterator->interactive || !iterator->enabled
            || iterator->opacity <= 0.0) {
            return {{false, 5403, QStringLiteral("Input target is unavailable")}, {}, false};
        }
        return {{true, 0, {}}, iterator->componentId, false};
    }
    for (const auto &target : targets_) {
        if (!target.visible || !target.interactive || !target.enabled || target.opacity <= 0.0 || !target.bounds.isValid()) continue;
        if (!hit(target, event.x, event.y)) continue;
        if (target.securityLayer) {
            return {{false, 5406, QStringLiteral("Input blocked by safety layer")}, target.componentId, true};
        }
        return {{true, 0, {}}, target.componentId, false};
    }
    return {{false, 5404, QStringLiteral("No input target was hit")}, {}, false};
}

bool InputRouter::hit(const HitTarget &target, double x, double y)
{
    if (target.shape == HitShape::Rectangle) return target.bounds.contains(x, y);
    if (target.shape == HitShape::Polygon) return target.polygon.containsPoint(QPointF {x, y}, Qt::OddEvenFill);
    const double radiusX = target.bounds.width / 2.0;
    const double radiusY = target.bounds.height / 2.0;
    const double normalizedX = (x - (target.bounds.x + radiusX)) / radiusX;
    const double normalizedY = (y - (target.bounds.y + radiusY)) / radiusY;
    return normalizedX * normalizedX + normalizedY * normalizedY <= 1.0;
}

} // namespace astra::ui
