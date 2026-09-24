#include "astra/ui/SpatialFocusManager.h"

#include <algorithm>

namespace astra::ui {

OperationResult SpatialFocusManager::registerTarget(const FocusTarget &target)
{
    if (target.componentId.isEmpty() || target.scopeId.isEmpty()) return {false, 5502, QStringLiteral("Invalid focus target")};
    targets_.insert(target.componentId, target);
    return {true, 0, {}};
}

OperationResult SpatialFocusManager::unregisterTarget(const QString &componentId)
{
    if (!targets_.contains(componentId)) return {false, 5501, QStringLiteral("Focus target not found")};
    const auto scope = targets_.value(componentId).scopeId;
    if (scopes_.value(scope).componentId == componentId) scopes_.remove(scope);
    targets_.remove(componentId);
    return {true, 0, {}};
}

OperationResult SpatialFocusManager::setAvailable(const QString &componentId, bool available)
{
    auto iterator = targets_.find(componentId);
    if (iterator == targets_.end()) return {false, 5501, QStringLiteral("Focus target not found")};
    iterator->visible = available;
    if (!available && scopes_.value(iterator->scopeId).componentId == componentId) scopes_.remove(iterator->scopeId);
    return {true, 0, {}};
}

OperationResult SpatialFocusManager::requestFocus(const QString &componentId, FocusType type, FocusPriority priority, const QString &reason)
{
    const auto target = targets_.value(componentId);
    if (target.componentId.isEmpty() || !target.visible || !target.interactive || !target.focusable) {
        return {false, 5501, QStringLiteral("Focus target is unavailable")};
    }
    const auto current = scopes_.value(target.scopeId);
    if (!current.componentId.isEmpty() && current.componentId != componentId
        && (static_cast<int>(priority) < static_cast<int>(current.priority)
            || (static_cast<int>(priority) == static_cast<int>(current.priority)
                && priority != FocusPriority::ActiveWindow && priority != FocusPriority::System))) {
        return {false, 5503, QStringLiteral("Focus preemption denied")};
    }
    scopes_.insert(target.scopeId, {componentId, type, priority, reason});
    return {true, 0, {}};
}

OperationResult SpatialFocusManager::releaseFocus(const QString &scopeId, const QString &reason)
{
    static_cast<void>(reason);
    if (scopeId.isEmpty()) return {false, 5502, QStringLiteral("Invalid focus scope")};
    scopes_.remove(scopeId);
    return {true, 0, {}};
}

OperationResult SpatialFocusManager::focusNext(const QString &scopeId)
{
    QList<FocusTarget> candidates;
    for (const auto &target : targets_) {
        if (target.scopeId == scopeId && target.visible && target.interactive && target.focusable) candidates.append(target);
    }
    if (candidates.isEmpty()) return {false, 5504, QStringLiteral("No focus target is available")};
    std::sort(candidates.begin(), candidates.end(), [](const FocusTarget &left, const FocusTarget &right) {
        if (left.stableOrder != right.stableOrder) return left.stableOrder < right.stableOrder;
        return left.componentId < right.componentId;
    });
    const QString current = owner(scopeId);
    qsizetype next = 0;
    for (qsizetype index = 0; index < candidates.size(); ++index) {
        if (candidates.at(index).componentId == current) next = (index + 1) % candidates.size();
    }
    scopes_.insert(scopeId, {candidates.at(next).componentId, FocusType::Keyboard, FocusPriority::NormalComponent, QStringLiteral("stable traversal")});
    return {true, 0, {}};
}

void SpatialFocusManager::clear()
{
    targets_.clear();
    scopes_.clear();
}

QString SpatialFocusManager::owner(const QString &scopeId) const { return scopes_.value(scopeId).componentId; }
FocusRecord SpatialFocusManager::record(const QString &scopeId) const { return scopes_.value(scopeId); }

} // namespace astra::ui
