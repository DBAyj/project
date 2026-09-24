#include "controllers/ShellController.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTemporaryDir>
#include <QtGlobal>

#include <cassert>

int main()
{
    QTemporaryDir directory;
    astra::shell::SimulatorConfig config;
    config.recentTaskLimit = 2;
    astra::shell::ShellController controller(config, directory.filePath("audit/audit.jsonl"));

    controller.submit(QStringLiteral("把设备模型投到桌面上"), QStringLiteral("PUBLIC"));
    assert(controller.projectionState() == QStringLiteral("ACTIVE"));
    assert(controller.projectionModel()->modelVisible());
    assert(!controller.projectionModel()->sessionId().isEmpty());
    assert(controller.recentTasks()->count() == 1);
    assert(controller.currentRawText() == QStringLiteral("把设备模型投到桌面上"));
    assert(controller.currentIntent() == QStringLiteral("project_3d_model"));
    assert(controller.currentTargetSpace() == QStringLiteral("desk"));
    assert(controller.currentPrivacyLevel() == QStringLiteral("PUBLIC"));
    assert(controller.currentTaskStatus() == QStringLiteral("ACTIVE"));
    assert(!controller.currentRequestId().isEmpty());

    controller.pauseRotation();
    assert(controller.projectionState() == QStringLiteral("PAUSED"));
    assert(controller.projectionModel()->rotationPaused());
    controller.resumeRotation();
    assert(controller.projectionState() == QStringLiteral("ACTIVE"));
    controller.changeZoom(0.5);
    assert(controller.projectionModel()->zoom() == 1.5);
    controller.rotateView(24.0);
    assert(controller.projectionModel()->sceneRotation() == 24.0);
    controller.resetView();
    assert(controller.projectionModel()->zoom() == 1.0);
    controller.enterFullscreen();
    assert(controller.projectionFullscreen());
    controller.exitFullscreen();
    assert(!controller.projectionFullscreen());

    controller.stopProjection();
    assert(controller.projectionState() == QStringLiteral("IDLE"));
    assert(!controller.projectionModel()->modelVisible());
    controller.submit(QStringLiteral("开始投影"), QStringLiteral("PRIVATE_SCREEN_ONLY"));
    assert(controller.projectionState() == QStringLiteral("IDLE"));
    assert(controller.lastErrorCode() == 4301);
    assert(controller.currentTaskStatus() == QStringLiteral("DENIED"));
    assert(!controller.projectionModel()->modelVisible());
    controller.submit(QStringLiteral("帮我订一张去上海的机票"), QStringLiteral("PUBLIC"));
    assert(controller.lastErrorCode() == 2001);
    assert(controller.recentTasks()->count() == 2);

    QFile audit(directory.filePath("audit/audit.jsonl"));
    assert(audit.open(QIODevice::ReadOnly));
    const QList<QByteArray> auditLines = audit.readAll().split('\n');
    assert(auditLines.size() >= 8);
    QString startTrace;
    QString startRequestedTrace;
    QString activeSession;
    for (const QByteArray &line : auditLines) {
        if (line.isEmpty()) continue;
        QJsonParseError parseError;
        const QJsonObject record = QJsonDocument::fromJson(line, &parseError).object();
        assert(parseError.error == QJsonParseError::NoError);
        for (const QString &field : {QStringLiteral("schema_version"), QStringLiteral("timestamp"), QStringLiteral("module"),
                                     QStringLiteral("trace_id"), QStringLiteral("request_id"), QStringLiteral("actor"),
                                     QStringLiteral("privacy_level"), QStringLiteral("result"), QStringLiteral("message")}) {
            assert(!record.value(field).toString().isEmpty());
        }
        if (record.value("event") == QStringLiteral("projection_started")) {
            startTrace = record.value("trace_id").toString();
            assert(startTrace == startRequestedTrace);
            activeSession = record.value("session_id").toString();
        }
        if (record.value("event") == QStringLiteral("projection_paused") || record.value("event") == QStringLiteral("projection_resumed")
            || record.value("event") == QStringLiteral("projection_stopped")) {
            assert(record.value("session_id").toString() == activeSession);
        }
        if (record.value("event") == QStringLiteral("projection_start_requested")) {
            startRequestedTrace = record.value("trace_id").toString();
        }
    }

    astra::shell::ShellController stability(config, directory.filePath("audit/stability.jsonl"));
    for (int cycle = 0; cycle < 100; ++cycle) {
        stability.submit(QStringLiteral("开始投影"), QStringLiteral("PUBLIC"));
        assert(stability.projectionState() == QStringLiteral("ACTIVE"));
        stability.pauseRotation();
        assert(stability.projectionState() == QStringLiteral("PAUSED"));
        stability.resumeRotation();
        assert(stability.projectionState() == QStringLiteral("ACTIVE"));
        stability.stopProjection();
        assert(stability.projectionState() == QStringLiteral("IDLE"));
        assert(!stability.projectionModel()->modelVisible());
    }

    const QString evidencePath = qEnvironmentVariable("ASTRA_AUDIT_EVIDENCE_PATH");
    if (!evidencePath.isEmpty()) {
        const QFileInfo evidenceInfo(evidencePath);
        assert(QDir().mkpath(evidenceInfo.dir().path()));
        QFile evidence(evidencePath);
        assert(evidence.open(QIODevice::WriteOnly | QIODevice::Append));
        for (const QString &sourceName : {QStringLiteral("audit/audit.jsonl"), QStringLiteral("audit/stability.jsonl")}) {
            QFile source(directory.filePath(sourceName));
            assert(source.open(QIODevice::ReadOnly));
            assert(evidence.write(source.readAll()) > 0);
        }
    }

    // P1-003: ROOM_ONLY stays projectable in the simulator although the policy library now defaults room trust to deny.
    astra::shell::ShellController roomController(config, directory.filePath("audit/room-only.jsonl"));
    roomController.submit(QStringLiteral("把设备模型投到桌面上"), QStringLiteral("ROOM_ONLY"));
    assert(roomController.projectionState() == QStringLiteral("ACTIVE"));
    assert(roomController.lastErrorCode() == 0);
}
