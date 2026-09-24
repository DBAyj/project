#include "services/ProjectionPolicyService.h"

#include "astra/common/ErrorCode.h"

namespace astra::shell {

ProjectionPolicyDecision ProjectionPolicyService::evaluate(const ProjectionPolicyRequest &request)
{
    using astra::common::PrivacyLevel;
    switch (request.privacyLevel) {
    case PrivacyLevel::Public:
    case PrivacyLevel::RoomOnly:
        return {true, 0, QStringLiteral("Projection allowed by privacy policy"), false, true};
    case PrivacyLevel::AuthorizedPerson:
        if (request.simulatedAuthorized) {
            return {true, 0, QStringLiteral("Authorized person verification succeeded"), false, true};
        }
        return {false, static_cast<int>(astra::common::ErrorCode::AuthorizationFailed),
                QStringLiteral("Authorized person verification failed"), true, true};
    case PrivacyLevel::PrivateScreenOnly:
    case PrivacyLevel::NoProjection:
        return {false, static_cast<int>(astra::common::ErrorCode::PrivacyPolicyDenied),
                QStringLiteral("Projection denied by privacy policy"), true, true};
    }
    return {false, static_cast<int>(astra::common::ErrorCode::PrivacyPolicyDenied),
            QStringLiteral("Projection denied by privacy policy"), true, true};
}

} // namespace astra::shell
