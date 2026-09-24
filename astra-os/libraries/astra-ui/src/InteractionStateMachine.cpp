#include "astra/ui/InteractionStateMachine.h"

#include <algorithm>
#include <cmath>

namespace astra::ui {
namespace {

bool isActive(InteractionState state)
{
    return state == InteractionState::Pressed || state == InteractionState::Selected || state == InteractionState::Dragging
        || state == InteractionState::Resizing || state == InteractionState::Rotating || state == InteractionState::Scaling;
}

bool allowed(InteractionState from, InteractionState to)
{
    using State = InteractionState;
    if (isActive(from) && to == State::Cancelled) return true;
    switch (from) {
    case State::Idle: return to == State::Hovered || to == State::Pressed || to == State::Disabled;
    case State::Hovered: return to == State::Idle || to == State::Pressed;
    case State::Pressed: return to == State::Selected || to == State::Dragging || to == State::Resizing || to == State::Rotating || to == State::Scaling;
    case State::Selected: return to == State::Idle || to == State::Pressed;
    case State::Dragging:
    case State::Resizing:
    case State::Rotating:
    case State::Scaling: return to == State::Idle || to == State::Error;
    case State::Disabled: return to == State::Idle;
    case State::Cancelled:
    case State::Error: return to == State::Idle;
    }
    return false;
}

} // namespace

InteractionState InteractionStateMachine::state() const { return state_; }

OperationResult InteractionStateMachine::transitionTo(InteractionState next)
{
    if (!allowed(state_, next)) return {false, 5601, QStringLiteral("Invalid interaction state transition")};
    state_ = next;
    return {true, 0, {}};
}

InteractionSession::InteractionSession(SpatialUIComponent &component, SpatialBounds safeArea)
    : component_(component)
    , safeArea_(safeArea)
    , initialBounds_(component.bounds())
    , initialTransform_(component.transform())
{
}

InteractionStateMachine &InteractionSession::machine() { return machine_; }

OperationResult InteractionSession::press()
{
    initialBounds_ = component_.bounds();
    initialTransform_ = component_.transform();
    return machine_.transitionTo(InteractionState::Pressed);
}

OperationResult InteractionSession::begin(InteractionState mode)
{
    if (mode != InteractionState::Dragging && mode != InteractionState::Resizing && mode != InteractionState::Rotating && mode != InteractionState::Scaling) {
        return {false, 5601, QStringLiteral("Unsupported interaction mode")};
    }
    return machine_.transitionTo(mode);
}

OperationResult InteractionSession::dragBy(double deltaX, double deltaY)
{
    if (machine_.state() != InteractionState::Dragging || !std::isfinite(deltaX) || !std::isfinite(deltaY)) {
        return {false, 5602, QStringLiteral("Invalid drag operation")};
    }
    auto bounds = initialBounds_;
    bounds.x += deltaX;
    bounds.y += deltaY;
    return component_.setBounds(clamp(bounds));
}

OperationResult InteractionSession::resizeBy(double deltaWidth, double deltaHeight)
{
    if (machine_.state() != InteractionState::Resizing || !std::isfinite(deltaWidth) || !std::isfinite(deltaHeight)) {
        return {false, 5603, QStringLiteral("Invalid resize operation")};
    }
    auto bounds = initialBounds_;
    bounds.width += deltaWidth;
    bounds.height += deltaHeight;
    if (bounds.width < 80.0 || bounds.height < 60.0) return {false, 5603, QStringLiteral("Resize is below minimum size")};
    return component_.setBounds(clamp(bounds));
}

OperationResult InteractionSession::rotateBy(double degrees)
{
    if (machine_.state() != InteractionState::Rotating || !std::isfinite(degrees)) return {false, 5604, QStringLiteral("Invalid rotation operation")};
    auto transform = initialTransform_;
    transform.rotationDegrees += degrees;
    return component_.setTransform(transform);
}

OperationResult InteractionSession::scaleBy(double factor)
{
    if (machine_.state() != InteractionState::Scaling || !std::isfinite(factor) || factor <= 0.0) return {false, 5603, QStringLiteral("Invalid scale operation")};
    auto transform = initialTransform_;
    transform.scale *= factor;
    return component_.setTransform(transform);
}

OperationResult InteractionSession::commit()
{
    const auto state = machine_.state();
    if (state != InteractionState::Dragging && state != InteractionState::Resizing && state != InteractionState::Rotating && state != InteractionState::Scaling) {
        return {false, 5601, QStringLiteral("No interaction is active")};
    }
    return machine_.transitionTo(InteractionState::Idle);
}

OperationResult InteractionSession::cancel()
{
    if (!isActive(machine_.state())) return {false, 5605, QStringLiteral("No interaction is active")};
    const auto boundsResult = component_.setBounds(initialBounds_);
    const auto transformResult = component_.setTransform(initialTransform_);
    if (!boundsResult.ok || !transformResult.ok) return {false, 5605, QStringLiteral("Interaction rollback failed")};
    return machine_.transitionTo(InteractionState::Cancelled);
}

OperationResult InteractionSession::reset() { return machine_.transitionTo(InteractionState::Idle); }

SpatialBounds InteractionSession::clamp(SpatialBounds bounds) const
{
    bounds.width = std::min(bounds.width, safeArea_.width);
    bounds.height = std::min(bounds.height, safeArea_.height);
    bounds.x = std::clamp(bounds.x, safeArea_.x, safeArea_.x + safeArea_.width - bounds.width);
    bounds.y = std::clamp(bounds.y, safeArea_.y, safeArea_.y + safeArea_.height - bounds.height);
    return bounds;
}

} // namespace astra::ui
