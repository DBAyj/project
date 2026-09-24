#include "astra/ui/PrivacyAwareUIService.h"

#include <QtGlobal>

using namespace astra::ui;

namespace {
void require(bool condition)
{
    if (!condition) qFatal("P5 privacy isolation requirement failed");
}
}

int main()
{
    const QString bindingKey = QStringLiteral("fixture-policy-binding-key");
    PrivacyAwareUIService privacy;
    ComponentSpec spec;
    spec.id = QStringLiteral("6f26be98-b772-48f1-8137-6fa449bd0ee2");
    spec.type = ComponentType::TaskCard;
    spec.bounds = {0.0, 0.0, 200.0, 100.0};
    spec.privacyLevel = astra::common::PrivacyLevel::PrivateScreenOnly;
    spec.accessibilityLabel = QStringLiteral("Private task");
    SpatialUIComponent component {spec};
    ProjectionLayerMapper mapper {privacy, bindingKey};
    require(component.transitionTo(ComponentLifecycleState::Attached).ok);
    require(component.transitionTo(ComponentLifecycleState::Visible).ok);
    auto deniedRequest = astra::policy::ProjectionPolicyRequest {astra::common::PrivacyLevel::PrivateScreenOnly};
    deniedRequest.subjectId = component.componentId();
    const auto denied = astra::policy::ProjectionPolicyService::evaluate(deniedRequest, bindingKey);
    require(privacy.present(component, denied) == VisibilityDecision::PhoneOnly);
    require(!mapper.map(component, denied).has_value());

    spec.id = QStringLiteral("04162f79-d8cc-4454-9c71-240852b87001");
    spec.privacyLevel = astra::common::PrivacyLevel::Public;
    SpatialUIComponent publicComponent {spec};
    require(publicComponent.transitionTo(ComponentLifecycleState::Attached).ok);
    require(publicComponent.transitionTo(ComponentLifecycleState::Visible).ok);
    auto allowedRequest = astra::policy::ProjectionPolicyRequest {astra::common::PrivacyLevel::Public};
    allowedRequest.subjectId = publicComponent.componentId();
    const auto allowed = astra::policy::ProjectionPolicyService::evaluate(allowedRequest, bindingKey);
    require(privacy.present(publicComponent, allowed) == VisibilityDecision::Visible);
    const auto layer = mapper.map(publicComponent, allowed);
    require(layer.has_value());
    require(layer->type == astra::render::LayerType::ApplicationSurface);
    require(layer->policyDecisionId == allowed.decisionId);

    require(!mapper.map(publicComponent, denied).has_value());
}
