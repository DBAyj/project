#pragma once

#include "astra/policy/ProjectionPolicyService.h"
#include "astra/render/ProjectionLayer.h"
#include "astra/ui/SpatialUIComponent.h"

#include <optional>

namespace astra::ui {

enum class VisibilityDecision { Visible, Redacted, PhoneOnly, Hidden, Denied };

class PrivacyAwareUIService final {
public:
    [[nodiscard]] VisibilityDecision present(
        const SpatialUIComponent &component, const astra::policy::ProjectionPolicyDecision &policyDecision) const;
};

class ProjectionLayerMapper final {
public:
    explicit ProjectionLayerMapper(const PrivacyAwareUIService &privacyService, QString policyBindingKey = {});
    [[nodiscard]] std::optional<astra::render::ProjectionLayer> map(
        const SpatialUIComponent &component, const astra::policy::ProjectionPolicyDecision &policyDecision) const;

private:
    const PrivacyAwareUIService &privacyService_;
    QString policyBindingKey_;
};

} // namespace astra::ui
