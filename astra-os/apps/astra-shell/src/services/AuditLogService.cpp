#include "services/AuditLogService.h"

#include "astra/common/Identifiers.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>

namespace astra::shell {
AuditLogService::AuditLogService(QString path) : path_(std::move(path)) {}
bool AuditLogService::write(const AuditEvent &event) const
{
    const QFileInfo info(path_);
    if (!QDir().mkpath(info.dir().path())) return false;
    QFile file(path_);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text)) return false;
    QJsonObject record {
        {"schema_version", event.schemaVersion},
        {"timestamp", event.timestamp.isEmpty() ? QString::fromStdString(astra::common::utcTimestamp()) : event.timestamp},
        {"level", event.errorCode == 0 ? event.level : QStringLiteral("WARN")},
        {"service", QStringLiteral("astra-shell")},
        {"module", event.module},
        {"event", event.event},
        {"trace_id", event.traceId.isEmpty() ? QString::fromStdString(astra::common::newUuid()) : event.traceId},
        {"request_id", event.requestId.isEmpty() ? QString::fromStdString(astra::common::newUuid()) : event.requestId},
        {"session_id", event.sessionId.isEmpty() ? QJsonValue::Null : QJsonValue(event.sessionId)},
        {"actor", event.actor},
        {"intent", event.intent},
        {"privacy_level", event.privacyLevel},
        {"result", event.result},
        {"error_code", event.errorCode == 0 ? QJsonValue::Null : QJsonValue(event.errorCode)},
        {"message", event.message.isEmpty() ? event.event : event.message},
        {"details", event.details},
    };
    return file.write(QJsonDocument(record).toJson(QJsonDocument::Compact) + '\n') > 0;
}

bool AuditLogService::write(const QString &event, const QString &requestId, const QString &intent, const QString &privacyLevel, const QString &result, int errorCode) const
{
    AuditEvent record;
    record.event = event;
    record.requestId = requestId;
    record.intent = intent;
    record.privacyLevel = privacyLevel;
    record.result = result;
    record.errorCode = errorCode;
    record.message = event;
    return write(record);
}
} // namespace astra::shell
