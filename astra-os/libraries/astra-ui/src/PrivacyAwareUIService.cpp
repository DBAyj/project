#include "astra/ui/PrivacyAwareUIService.h"

#include <utility>

namespace astra::ui {
namespace {

astra::render::LayerType layerType(ComponentType componentType)
{
    using astra::render::LayerType;
    switch (componentType) {
    case ComponentType::Notification:
    case ComponentType::SystemCriticalAlert:
    case ComponentType::PrivacyBadge: return LayerType::Notification;
    case ComponentType::DebugPanel: return LayerType::DebugOverlay;
    case ComponentType::ModelView: return LayerType::Scene3D;
    default: return LayerType::ApplicationSurface;
    }
}

} // namespace

VisibilityDecision PrivacyAwareUIService::present(
    const SpatialUIComponent &component, const astra::policy::ProjectionPolicyDecision &policyDecision) const
{
    if (!component.isVisible()) return VisibilityDecision::Hidden;
    if (policyDecision.allowed) return VisibilityDecision::Visible;
    if (component.privacyLevel() == astra::common::PrivacyLevel::PrivateScreenOnly) return VisibilityDecision::PhoneOnly;
    return VisibilityDecision::Denied;
}

ProjectionLayerMapper::ProjectionLayerMapper(const PrivacyAwareUIService &privacyService, QString policyBindingKey)
    : privacyService_(privacyService)
    , policyBindingKey_(std::move(policyBindingKey))
{
}

std::optional<astra::render::ProjectionLayer> ProjectionLayerMapper::map(
    const SpatialUIComponent &component, const astra::policy::ProjectionPolicyDecision &policyDecision) const
{
    const auto decision = privacyService_.present(component, policyDecision);
    if (decision != VisibilityDecision::Visible && decision != VisibilityDecision::Redacted) return std::nullopt;
    if (!policyDecision.allowed || policyDecision.subjectId != component.componentId()
        || policyDecision.privacyLevel != component.privacyLevel()
        || !astra::policy::ProjectionPolicyService::matchesBinding(policyDecision.decisionId,
                                                                    component.componentId(),
                                                                    component.privacyLevel(),
                                                                    true,
                                                                    policyBindingKey_)) {
        return std::nullopt;
    }
    if (component.componentType() == ComponentType::PrivacyMask) return std::nullopt;
    return astra::render::ProjectionLayer {component.componentId(), layerType(component.componentType()), component.zOrder(), true,
                                           component.privacyLevel(),
                                           QRectF {component.bounds().x, component.bounds().y, component.bounds().width, component.bounds().height},
                                           component.accessibilityLabel().left(80), policyDecision.decisionId,
                                           policyDecision.subjectId};
}

} // namespace astra::ui
