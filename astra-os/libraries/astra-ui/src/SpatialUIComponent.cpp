#include "astra/ui/SpatialUIComponent.h"

#include <cmath>

namespace astra::ui {
namespace {

bool isAllowed(ComponentLifecycleState from, ComponentLifecycleState to)
{
    using State = ComponentLifecycleState;
    switch (from) {
    case State::Created: return to == State::Attached;
    case State::Attached: return to == State::Visible;
    case State::Visible: return to == State::Focused || to == State::Hidden || to == State::Suspended || to == State::Detached;
    case State::Focused: return to == State::Interacting || to == State::Visible;
    case State::Interacting: return to == State::Focused;
    case State::Hidden: return to == State::Visible || to == State::Detached;
    case State::Suspended: return to == State::Visible;
    case State::Detached: return to == State::Destroyed;
    case State::Error: return to == State::Detached;
    case State::Destroyed: return false;
    }
    return false;
}

} // namespace

SpatialUIComponent::SpatialUIComponent(ComponentSpec spec)
    : spec_(std::move(spec))
    , createdAt_(QDateTime::currentDateTimeUtc())
    , updatedAt_(createdAt_)
{
}

QString SpatialUIComponent::componentId() const { return spec_.id; }
ComponentType SpatialUIComponent::componentType() const { return spec_.type; }
QString SpatialUIComponent::parentId() const { return spec_.parentId; }
QStringList SpatialUIComponent::children() const { return spec_.children; }
SpatialBounds SpatialUIComponent::bounds() const { return spec_.bounds; }
SpatialTransform SpatialUIComponent::transform() const { return spec_.transform; }
int SpatialUIComponent::zOrder() const { return spec_.zOrder; }
double SpatialUIComponent::opacity() const { return spec_.opacity; }
astra::common::PrivacyLevel SpatialUIComponent::privacyLevel() const { return spec_.privacyLevel; }
DisplayTarget SpatialUIComponent::displayTarget() const { return spec_.displayTarget; }
bool SpatialUIComponent::isVisible() const { return lifecycleState_ == ComponentLifecycleState::Visible || lifecycleState_ == ComponentLifecycleState::Focused || lifecycleState_ == ComponentLifecycleState::Interacting; }
bool SpatialUIComponent::isEnabled() const { return spec_.enabled; }
bool SpatialUIComponent::isFocusable() const { return spec_.focusable; }
bool SpatialUIComponent::isInteractive() const { return spec_.interactive; }
QString SpatialUIComponent::accessibilityLabel() const { return spec_.accessibilityLabel; }
ComponentLifecycleState SpatialUIComponent::lifecycleState() const { return lifecycleState_; }
QDateTime SpatialUIComponent::createdAt() const { return createdAt_; }
QDateTime SpatialUIComponent::updatedAt() const { return updatedAt_; }

OperationResult SpatialUIComponent::transitionTo(ComponentLifecycleState next)
{
    if (!isAllowed(lifecycleState_, next)) return {false, 5107, QStringLiteral("Invalid component lifecycle transition")};
    lifecycleState_ = next;
    updatedAt_ = QDateTime::currentDateTimeUtc();
    return {true, 0, {}};
}

OperationResult SpatialUIComponent::setBounds(const SpatialBounds &bounds)
{
    if (!bounds.isValid()) return {false, 5205, QStringLiteral("Invalid component bounds")};
    spec_.bounds = bounds;
    updatedAt_ = QDateTime::currentDateTimeUtc();
    return {true, 0, {}};
}

OperationResult SpatialUIComponent::setTransform(const SpatialTransform &transform)
{
    if (!std::isfinite(transform.scale) || transform.scale <= 0.0) return {false, 5603, QStringLiteral("Invalid component transform")};
    spec_.transform = transform;
    updatedAt_ = QDateTime::currentDateTimeUtc();
    return {true, 0, {}};
}

OperationResult SpatialUIComponent::setParentId(const QString &parentId)
{
    if (parentId == spec_.id) return {false, 5103, QStringLiteral("A component cannot parent itself")};
    spec_.parentId = parentId;
    updatedAt_ = QDateTime::currentDateTimeUtc();
    return {true, 0, {}};
}

OperationResult SpatialUIComponent::addChild(const QString &childId)
{
    if (childId.isEmpty() || childId == spec_.id || spec_.children.contains(childId)) {
        return {false, 5103, QStringLiteral("Invalid component child")};
    }
    spec_.children.append(childId);
    updatedAt_ = QDateTime::currentDateTimeUtc();
    return {true, 0, {}};
}

void SpatialUIComponent::removeChild(const QString &childId)
{
    if (spec_.children.removeAll(childId) > 0) updatedAt_ = QDateTime::currentDateTimeUtc();
}

void SpatialUIComponent::setPrivacyLevel(astra::common::PrivacyLevel level)
{
    spec_.privacyLevel = level;
    updatedAt_ = QDateTime::currentDateTimeUtc();
}

void SpatialUIComponent::setDisplayTarget(DisplayTarget target)
{
    spec_.displayTarget = target;
    updatedAt_ = QDateTime::currentDateTimeUtc();
}

void SpatialUIComponent::setAccessibilityLabel(QString label)
{
    spec_.accessibilityLabel = std::move(label);
    updatedAt_ = QDateTime::currentDateTimeUtc();
}

} // namespace astra::ui
