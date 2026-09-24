#include "services/AuditLogService.h"

#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTemporaryDir>
#include <cassert>

int main()
{
    QTemporaryDir directory;
    const QString path = directory.filePath("audit/events.jsonl");
    astra::shell::AuditLogService audit(path);
    astra::shell::AuditEvent event;
    event.traceId = QStringLiteral("7abf7b1e-dc4f-4b7c-96cc-c3dd628e1ca9");
    event.event = QStringLiteral("projection_started");
    event.requestId = QStringLiteral("request-1");
    event.sessionId = QStringLiteral("session-1");
    event.intent = QStringLiteral("project_3d_model");
    event.privacyLevel = QStringLiteral("PUBLIC");
    event.result = QStringLiteral("success");
    event.module = QStringLiteral("projection.session");
    event.message = QStringLiteral("Projection session started");
    event.details.insert(QStringLiteral("target_space"), QStringLiteral("desk"));
    assert(audit.write(event));
    QFile file(path);
    assert(file.open(QIODevice::ReadOnly));
    QJsonParseError error;
    const auto document = QJsonDocument::fromJson(file.readLine(), &error);
    assert(error.error == QJsonParseError::NoError);
    const auto record = document.object();
    assert(record.value("event") == QStringLiteral("projection_started"));
    assert(record.value("schema_version") == QStringLiteral("1.0"));
    assert(record.value("trace_id") == event.traceId);
    assert(record.value("module") == event.module);
    assert(record.value("message") == event.message);
    assert(record.value("actor") == QStringLiteral("local-user"));
    assert(record.value("request_id") == QStringLiteral("request-1"));
    assert(record.value("session_id") == QStringLiteral("session-1"));
    assert(record.value("details").toObject().value("target_space") == QStringLiteral("desk"));
}
