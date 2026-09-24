#include "astra/policy/ProjectionPolicyService.h"

#include <QUuid>

#include <cassert>

int main()
{
    using astra::common::PrivacyLevel;
    using astra::policy::ProjectionPolicyRequest;
    using astra::policy::ProjectionPolicyService;

    const auto evaluate = [](PrivacyLevel privacyLevel, bool authorized = false, bool roomTrusted = true,
                             bool targetEnabled = true, bool targetAvailable = true) {
        ProjectionPolicyRequest request {privacyLevel, authorized};
        request.roomTrusted = roomTrusted;
        request.projectionTargetEnabled = targetEnabled;
        request.targetAvailable = targetAvailable;
        return ProjectionPolicyService::evaluate(request);
    };

    const auto publicDecision = evaluate(PrivacyLevel::Public);
    assert(publicDecision.allowed && !QUuid {publicDecision.decisionId}.isNull());
    assert(evaluate(PrivacyLevel::RoomOnly).allowed);
    assert(!evaluate(PrivacyLevel::RoomOnly, false, false).allowed);
    assert(!ProjectionPolicyService::evaluate(ProjectionPolicyRequest {PrivacyLevel::RoomOnly}).allowed);
    assert(!evaluate(PrivacyLevel::AuthorizedPerson).allowed);
    assert(evaluate(PrivacyLevel::AuthorizedPerson, true).allowed);
    assert(!evaluate(PrivacyLevel::PrivateScreenOnly).allowed);
    assert(!evaluate(PrivacyLevel::NoProjection).allowed);
    assert(!evaluate(PrivacyLevel::Public, false, true, false).allowed);
    assert(!evaluate(PrivacyLevel::Public, false, true, true, false).allowed);
    const QString subjectId = QStringLiteral("7abf7b1e-dc4f-4b7c-96cc-c3dd628e1ca9");
    const QString bindingKey = QStringLiteral("fixture-policy-binding-key");
    const auto classified = ProjectionPolicyService::classifyFixture(
        subjectId, PrivacyLevel::Public, PrivacyLevel::PrivateScreenOnly, bindingKey);
    assert(classified.privacyLevel == PrivacyLevel::PrivateScreenOnly);
    const auto fixturePublic = ProjectionPolicyService::classifyFixture(
        subjectId, PrivacyLevel::Public, PrivacyLevel::Public, bindingKey);
    assert(fixturePublic.privacyLevel == PrivacyLevel::Public);
    const auto bound = ProjectionPolicyService::evaluate(
        [&] { ProjectionPolicyRequest request {PrivacyLevel::Public}; request.subjectId = subjectId; return request; }(), bindingKey);
    assert(ProjectionPolicyService::matchesBinding(bound.decisionId, subjectId, PrivacyLevel::Public, true, bindingKey));
    assert(!ProjectionPolicyService::matchesBinding(bound.decisionId, subjectId, PrivacyLevel::Public, true,
                                                     QStringLiteral("different-key")));
}
