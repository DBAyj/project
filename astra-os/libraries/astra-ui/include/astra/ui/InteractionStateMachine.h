#pragma once

#include "astra/ui/SpatialUIComponent.h"

namespace astra::ui {

enum class InteractionState { Idle, Hovered, Pressed, Selected, Dragging, Resizing, Rotating, Scaling, Disabled, Cancelled, Error };

class InteractionStateMachine final {
public:
    [[nodiscard]] InteractionState state() const;
    OperationResult transitionTo(InteractionState next);

private:
    InteractionState state_ {InteractionState::Idle};
};

class InteractionSession final {
public:
    InteractionSession(SpatialUIComponent &component, SpatialBounds safeArea);

    [[nodiscard]] InteractionStateMachine &machine();
    OperationResult press();
    OperationResult begin(InteractionState mode);
    OperationResult dragBy(double deltaX, double deltaY);
    OperationResult resizeBy(double deltaWidth, double deltaHeight);
    OperationResult rotateBy(double degrees);
    OperationResult scaleBy(double factor);
    OperationResult commit();
    OperationResult cancel();
    OperationResult reset();

private:
    [[nodiscard]] SpatialBounds clamp(SpatialBounds bounds) const;

    SpatialUIComponent &component_;
    SpatialBounds safeArea_;
    SpatialBounds initialBounds_;
    SpatialTransform initialTransform_;
    InteractionStateMachine machine_;
};

} // namespace astra::ui
