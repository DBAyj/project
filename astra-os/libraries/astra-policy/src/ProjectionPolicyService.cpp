#include "astra/policy/ProjectionPolicyService.h"

#include "astra/common/ErrorCode.h"
#include "astra/common/Identifiers.h"
#include "astra/common/PrivacyLevel.h"

#include <QMessageAuthenticationCode>
#include <QCryptographicHash>
#include <QUuid>

namespace astra::policy {
namespace {

ProjectionPolicyDecision decision(const ProjectionPolicyRequest &request,
                                  bool allowed,
                                  int errorCode,
                                  const QString &reason,
                                  bool clearProjectionContent,
                                  const QString &bindingKey)
{
    const QString id = request.subjectId.isEmpty() || bindingKey.isEmpty()
        ? QString::fromStdString(astra::common::newUuid())
        : ProjectionPolicyService::bindingId(request.subjectId, request.privacyLevel, allowed, bindingKey);
    return {id, allowed, errorCode, reason, clearProjectionContent, true, request.subjectId, request.privacyLevel};
}

ProjectionPolicyDecision denied(const ProjectionPolicyRequest &request, const QString &reason, const QString &bindingKey)
{
    return decision(request, false, static_cast<int>(astra::common::ErrorCode::PrivacyPolicyDenied), reason, true, bindingKey);
}

int disclosureRank(astra::common::PrivacyLevel level)
{
    using astra::common::PrivacyLevel;
    switch (level) {
    case PrivacyLevel::Public: return 0;
    case PrivacyLevel::RoomOnly: return 1;
    case PrivacyLevel::AuthorizedPerson: return 2;
    case PrivacyLevel::PrivateScreenOnly: return 3;
    case PrivacyLevel::NoProjection: return 4;
    }
    return 4;
}

} // namespace

ProjectionPolicyDecision ProjectionPolicyService::evaluate(const ProjectionPolicyRequest &request, const QString &bindingKey)
{
    using astra::common::PrivacyLevel;
    if (!request.projectionTargetEnabled || !request.targetAvailable) {
        return denied(request, QStringLiteral("Projection target is unavailable or disabled by policy context"), bindingKey);
    }
    switch (request.privacyLevel) {
    case PrivacyLevel::Public:
        return decision(request, true, 0, QStringLiteral("Public projection allowed by astra-policy"), false, bindingKey);
    case PrivacyLevel::RoomOnly:
        return request.roomTrusted
            ? decision(request, true, 0, QStringLiteral("Room projection allowed by astra-policy"), false, bindingKey)
            : denied(request, QStringLiteral("Room trust is not established"), bindingKey);
    case PrivacyLevel::AuthorizedPerson:
        if (request.authorizedPersonPresent) {
            return decision(request, true, 0, QStringLiteral("Authorized person verification succeeded"), false, bindingKey);
        }
        return decision(request, false, static_cast<int>(astra::common::ErrorCode::AuthorizationFailed),
                        QStringLiteral("Authorized person verification failed"), true, bindingKey);
    case PrivacyLevel::PrivateScreenOnly:
    case PrivacyLevel::NoProjection:
        return denied(request, QStringLiteral("Projection denied by privacy policy"), bindingKey);
    }
    return denied(request, QStringLiteral("Projection denied for an unknown privacy level"), bindingKey);
}

ProjectionPolicyDecision ProjectionPolicyService::classifyFixture(const QString &subjectId,
                                                                  astra::common::PrivacyLevel requestedLevel,
                                                                  astra::common::PrivacyLevel maximumDisclosureLevel,
                                                                  const QString &bindingKey)
{
    const auto assignedLevel = disclosureRank(requestedLevel) < disclosureRank(maximumDisclosureLevel)
        ? maximumDisclosureLevel : requestedLevel;
    ProjectionPolicyRequest request {assignedLevel};
    request.subjectId = subjectId;
    return decision(request, true, 0, QStringLiteral("Fixture classification issued by astra-policy"), false, bindingKey);
}

QString ProjectionPolicyService::bindingId(const QString &subjectId,
                                           astra::common::PrivacyLevel privacyLevel,
                                           bool allowed,
                                           const QString &bindingKey)
{
    const QByteArray input = subjectId.toUtf8() + ':'
        + QByteArray {astra::common::privacyLevelToString(privacyLevel).data()} + ':'
        + (allowed ? QByteArrayLiteral("allow") : QByteArrayLiteral("deny"));
    QByteArray bytes = QMessageAuthenticationCode::hash(input, bindingKey.toUtf8(), QCryptographicHash::Sha256).left(16);
    bytes[6] = static_cast<char>((static_cast<unsigned char>(bytes.at(6)) & 0x0fU) | 0x50U);
    bytes[8] = static_cast<char>((static_cast<unsigned char>(bytes.at(8)) & 0x3fU) | 0x80U);
    return QUuid::fromRfc4122(bytes).toString(QUuid::WithoutBraces);
}

bool ProjectionPolicyService::matchesBinding(const QString &decisionId,
                                             const QString &subjectId,
                                             astra::common::PrivacyLevel privacyLevel,
                                             bool allowed,
                                             const QString &bindingKey)
{
    return !subjectId.isEmpty() && !bindingKey.isEmpty()
        && decisionId == bindingId(subjectId, privacyLevel, allowed, bindingKey);
}

} // namespace astra::policy
