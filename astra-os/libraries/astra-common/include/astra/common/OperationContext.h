#pragma once

#include <QString>

namespace astra::common {

struct OperationContext {
    QString traceId;
    QString requestId;
    QString sessionId;
    QString actor {QStringLiteral("local-user")};
    QString privacyLevel {QStringLiteral("PUBLIC")};
};

} // namespace astra::common
