#pragma once

#include "astra/common/ErrorCode.h"

#include <QString>

namespace astra::shell {
struct ProjectionOperationResult {
    bool ok {false};
    int errorCode {0};
    QString message;
    QString sessionId;
    QString state;
};
class ProjectionSessionService {
public:
    ProjectionOperationResult start();
    ProjectionOperationResult pause();
    ProjectionOperationResult resume();
    ProjectionOperationResult stop();
    QString state() const;
    QString sessionId() const;
private:
    ProjectionOperationResult failure(astra::common::ErrorCode code) const;
    ProjectionOperationResult success() const;
    QString state_ {"IDLE"};
    QString sessionId_;
};
} // namespace astra::shell
