#pragma once

#include "astra/common/PrivacyLevel.h"

#include <QString>

#include <utility>

namespace astra::policy {

enum class ProjectionAction { Start };

struct ProjectionPolicyRequest {
    ProjectionPolicyRequest(astra::common::PrivacyLevel level,
                            bool authorized = false,
                            ProjectionAction requestedAction = ProjectionAction::Start,
                            QString state = {})
        : privacyLevel(level)
        , authorizedPersonPresent(authorized)
        , action(requestedAction)
        , currentState(std::move(state))
    {
    }

    astra::common::PrivacyLevel privacyLevel;
    bool authorizedPersonPresent {false};
    ProjectionAction action {ProjectionAction::Start};
    QString currentState;
    bool roomTrusted {true};
    bool projectionTargetEnabled {true};
    bool targetAvailable {true};
    QString subjectId;
};

struct ProjectionPolicyDecision {
    QString decisionId;
    bool allowed {false};
    int errorCode {0};
    QString reason;
    bool clearProjectionContent {false};
    bool auditRequired {true};
    QString subjectId;
    astra::common::PrivacyLevel privacyLevel {astra::common::PrivacyLevel::NoProjection};
};

class ProjectionPolicyService final {
public:
    [[nodiscard]] static ProjectionPolicyDecision evaluate(const ProjectionPolicyRequest &request,
                                                             const QString &bindingKey = {});
    [[nodiscard]] static ProjectionPolicyDecision classifyFixture(const QString &subjectId,
                                                                   astra::common::PrivacyLevel requestedLevel,
                                                                   astra::common::PrivacyLevel maximumDisclosureLevel,
                                                                   const QString &bindingKey = {});
    [[nodiscard]] static QString bindingId(const QString &subjectId,
                                           astra::common::PrivacyLevel privacyLevel,
                                           bool allowed,
                                           const QString &bindingKey);
    [[nodiscard]] static bool matchesBinding(const QString &decisionId,
                                             const QString &subjectId,
                                             astra::common::PrivacyLevel privacyLevel,
                                             bool allowed,
                                             const QString &bindingKey);
};

} // namespace astra::policy
