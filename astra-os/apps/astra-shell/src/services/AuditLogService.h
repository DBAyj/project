#pragma once

#include <QJsonObject>
#include <QString>

namespace astra::shell {

struct AuditEvent {
    QString schemaVersion {QStringLiteral("1.0")};
    QString timestamp;
    QString level {QStringLiteral("INFO")};
    QString module {QStringLiteral("shell.audit")};
    QString event;
    QString traceId;
    QString requestId;
    QString sessionId;
    QString actor {QStringLiteral("local-user")};
    QString intent;
    QString privacyLevel;
    QString result;
    int errorCode {0};
    QString message;
    QJsonObject details;
};

class AuditLogService {
public:
    explicit AuditLogService(QString path);
    bool write(const AuditEvent &event) const;
    bool write(const QString &event, const QString &requestId, const QString &intent, const QString &privacyLevel, const QString &result, int errorCode) const;
private:
    QString path_;
};
} // namespace astra::shell
