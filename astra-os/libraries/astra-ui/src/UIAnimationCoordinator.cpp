#include "astra/ui/UIAnimationCoordinator.h"

namespace astra::ui {

void UIAnimationCoordinator::setReduceMotion(bool enabled) { reduceMotion_ = enabled; }
void UIAnimationCoordinator::setAnimationsEnabled(bool enabled) { animationsEnabled_ = enabled; }

AnimationStartResult UIAnimationCoordinator::start(const QString &componentId, AnimationType type, int durationMs, bool privacyImmediate)
{
    if (componentId.isEmpty() || durationMs < 0) return {{false, 5903, QStringLiteral("Invalid animation")}, 0};
    const quint64 id = nextId_++;
    const int effectiveDuration = (!animationsEnabled_ || reduceMotion_ || privacyImmediate) ? 0 : durationMs;
    const TransitionState state = effectiveDuration == 0 ? TransitionState::Completed : TransitionState::Running;
    transitions_.insert(id, {id, componentId, type, effectiveDuration, state});
    return {{true, 0, {}}, id};
}

OperationResult UIAnimationCoordinator::cancel(quint64 transitionId)
{
    auto iterator = transitions_.find(transitionId);
    if (iterator == transitions_.end() || iterator->state != TransitionState::Running) return {false, 5903, QStringLiteral("Animation cannot be cancelled")};
    iterator->state = TransitionState::Cancelled;
    return {true, 0, {}};
}

OperationResult UIAnimationCoordinator::complete(quint64 transitionId)
{
    auto iterator = transitions_.find(transitionId);
    if (iterator == transitions_.end() || iterator->state != TransitionState::Running) return {false, 5903, QStringLiteral("Animation cannot be completed")};
    iterator->state = TransitionState::Completed;
    return {true, 0, {}};
}

const UITransition *UIAnimationCoordinator::transition(quint64 transitionId) const
{
    const auto iterator = transitions_.constFind(transitionId);
    return iterator == transitions_.constEnd() ? nullptr : &iterator.value();
}

} // namespace astra::ui
