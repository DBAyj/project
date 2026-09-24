#include "astra/ui/InteractionStateMachine.h"

#include <QtGlobal>

using namespace astra::ui;

namespace {
void require(bool condition)
{
    if (!condition) qFatal("P5 interaction state requirement failed");
}
}

int main()
{
    ComponentSpec spec;
    spec.id = QStringLiteral("window-component");
    spec.type = ComponentType::SpatialWindow;
    spec.bounds = {10.0, 10.0, 200.0, 120.0};
    spec.accessibilityLabel = QStringLiteral("Window");
    SpatialUIComponent component {spec};
    InteractionSession session {component, {0.0, 0.0, 1000.0, 800.0}};

    require(!session.machine().transitionTo(InteractionState::Dragging).ok);
    require(session.machine().state() == InteractionState::Idle);
    require(session.press().ok);
    require(session.begin(InteractionState::Dragging).ok);
    require(session.dragBy(2000.0, 2000.0).ok);
    require(component.bounds().x == 800.0 && component.bounds().y == 680.0);
    require(session.cancel().ok);
    require(component.bounds().x == 10.0 && component.bounds().y == 10.0);
    require(session.machine().state() == InteractionState::Cancelled);
    require(session.reset().ok);

    require(session.press().ok);
    require(session.begin(InteractionState::Resizing).ok);
    const auto invalid = session.resizeBy(-500.0, -500.0);
    require(!invalid.ok && invalid.errorCode == 5603);
    require(component.bounds().width == 200.0);
    require(session.resizeBy(100.0, 80.0).ok);
    require(session.commit().ok);
    require(component.bounds().width == 300.0 && component.bounds().height == 200.0);
}
