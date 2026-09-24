#pragma once

#include "astra/ui/Types.h"

#include <QHash>

namespace astra::ui {

enum class AnimationType { Fade, Slide, Scale, Rotate, LayoutTransition, FocusHighlight };
enum class TransitionState { Running, Completed, Cancelled, Failed };

struct UITransition {
    quint64 transitionId {0};
    QString componentId;
    AnimationType type {AnimationType::Fade};
    int effectiveDurationMs {0};
    TransitionState state {TransitionState::Running};
};

struct AnimationStartResult : OperationResult {
    quint64 transitionId {0};
};

class UIAnimationCoordinator final {
public:
    void setReduceMotion(bool enabled);
    void setAnimationsEnabled(bool enabled);
    AnimationStartResult start(const QString &componentId, AnimationType type, int durationMs, bool privacyImmediate);
    OperationResult cancel(quint64 transitionId);
    OperationResult complete(quint64 transitionId);
    [[nodiscard]] const UITransition *transition(quint64 transitionId) const;

private:
    bool reduceMotion_ {false};
    bool animationsEnabled_ {true};
    quint64 nextId_ {1};
    QHash<quint64, UITransition> transitions_;
};

} // namespace astra::ui
