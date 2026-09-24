#include "services/ProjectionSessionService.h"

#include "astra/common/Identifiers.h"

namespace astra::shell {
ProjectionOperationResult ProjectionSessionService::failure(astra::common::ErrorCode code) const
{
    return {false, static_cast<int>(code), astra::common::errorMessage(code), sessionId_, state_};
}

ProjectionOperationResult ProjectionSessionService::success() const { return {true, 0, {}, sessionId_, state_}; }

ProjectionOperationResult ProjectionSessionService::start()
{
    if (state_ != QStringLiteral("IDLE")) return failure(astra::common::ErrorCode::ProjectionSessionExists);
    state_ = QStringLiteral("STARTING");
    sessionId_ = QString::fromStdString(astra::common::newUuid());
    state_ = QStringLiteral("ACTIVE");
    return success();
}
ProjectionOperationResult ProjectionSessionService::pause()
{
    if (state_ != QStringLiteral("ACTIVE")) return failure(astra::common::ErrorCode::InvalidProjectionTransition);
    state_ = QStringLiteral("PAUSED");
    return success();
}
ProjectionOperationResult ProjectionSessionService::resume()
{
    if (state_ != QStringLiteral("PAUSED")) return failure(astra::common::ErrorCode::InvalidProjectionTransition);
    state_ = QStringLiteral("ACTIVE");
    return success();
}
ProjectionOperationResult ProjectionSessionService::stop()
{
    if (state_ != QStringLiteral("ACTIVE") && state_ != QStringLiteral("PAUSED")) return failure(astra::common::ErrorCode::ProjectionSessionMissing);
    state_ = QStringLiteral("STOPPING");
    state_ = QStringLiteral("IDLE");
    sessionId_.clear();
    return success();
}
QString ProjectionSessionService::state() const { return state_; }
QString ProjectionSessionService::sessionId() const { return sessionId_; }
} // namespace astra::shell
