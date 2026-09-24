#include "services/ProjectionPolicyService.h"

#include <cassert>

int main()
{
    using astra::common::PrivacyLevel;
    using astra::shell::ProjectionAction;
    using astra::shell::ProjectionPolicyRequest;
    using astra::shell::ProjectionPolicyService;

    const auto allowed = [](PrivacyLevel privacyLevel, bool authorized) {
        ProjectionPolicyRequest request {privacyLevel, authorized, ProjectionAction::Start, QStringLiteral("IDLE")};
        request.roomTrusted = true;
        return ProjectionPolicyService::evaluate(request);
    };

    assert(allowed(PrivacyLevel::Public, false).allowed);
    assert(allowed(PrivacyLevel::RoomOnly, false).allowed);
    assert(!allowed(PrivacyLevel::AuthorizedPerson, false).allowed);
    assert(allowed(PrivacyLevel::AuthorizedPerson, true).allowed);
    const auto privateScreen = allowed(PrivacyLevel::PrivateScreenOnly, false);
    assert(!privateScreen.allowed && privateScreen.clearProjectionContent && privateScreen.errorCode == 4301);
    const auto noProjection = allowed(PrivacyLevel::NoProjection, false);
    assert(!noProjection.allowed && noProjection.clearProjectionContent && noProjection.errorCode == 4301);
    const auto unknown = allowed(static_cast<PrivacyLevel>(999), false);
    assert(!unknown.allowed && unknown.errorCode == 4301);
}
