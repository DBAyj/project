#include "astra/ui/AccessibilitySemanticService.h"
#include "astra/ui/UIAnimationCoordinator.h"

#include <QtGlobal>

using namespace astra::ui;

namespace {
void require(bool condition)
{
    if (!condition) qFatal("P5 animation or accessibility requirement failed");
}
}

int main()
{
    UIAnimationCoordinator animations;
    auto started = animations.start(QStringLiteral("card"), AnimationType::Slide, 220, false);
    require(started.ok && started.transitionId > 0);
    require(animations.transition(started.transitionId)->state == TransitionState::Running);
    require(animations.cancel(started.transitionId).ok);
    require(animations.transition(started.transitionId)->state == TransitionState::Cancelled);

    animations.setReduceMotion(true);
    started = animations.start(QStringLiteral("private-card"), AnimationType::Fade, 220, true);
    require(started.ok);
    require(animations.transition(started.transitionId)->effectiveDurationMs == 0);
    require(animations.transition(started.transitionId)->state == TransitionState::Completed);

    AccessibilitySemanticService semantics;
    SemanticNode button {QStringLiteral("confirm"), SemanticRole::Button, QStringLiteral("Confirm"),
                         QStringLiteral("Confirm task"), QStringLiteral("enabled"), {}, {QStringLiteral("press")}, true, true};
    require(semantics.registerNode(button).ok);
    require(semantics.invokeAction(button.componentId, QStringLiteral("press")).ok);
    SemanticNode invalid = button;
    invalid.componentId = QStringLiteral("unnamed");
    invalid.name.clear();
    require(semantics.registerNode(invalid).errorCode == 5901);
    require(semantics.setVisible(button.componentId, false).ok);
    require(semantics.visibleTree().isEmpty());
}
