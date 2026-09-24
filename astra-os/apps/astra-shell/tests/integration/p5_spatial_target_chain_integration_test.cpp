#include "astra/common/CapabilityToken.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLocalServer>
#include <QLocalSocket>
#include <QProcess>
#include <QTemporaryDir>
#include <QThread>
#include <QUuid>
#include <QtGlobal>

namespace {

enum class ServiceKind { Spatial, Projection, SpatialUI };

void require(bool condition, const char *message)
{
    if (!condition) qFatal("P5 P3/P4/P5 target-chain requirement failed: %s", message);
}

QString randomToken(const QString &prefix)
{
    return prefix + QUuid::createUuid().toString(QUuid::WithoutBraces) + QUuid::createUuid().toString(QUuid::WithoutBraces);
}

void writeToken(const QString &path, const QString &token)
{
    QFile file {path};
    require(file.open(QIODevice::WriteOnly | QIODevice::Truncate), "token file opened");
    require(file.write(token.toUtf8() + '\n') > 0, "token file written");
    file.close();
    require(QFile::setPermissions(path, QFileDevice::ReadOwner | QFileDevice::WriteOwner), "token permissions restricted");
}

QJsonObject rpc(const QString &socketPath, const QString &method, const QJsonObject &params,
                const QString &token, ServiceKind kind)
{
    QLocalSocket socket;
    socket.connectToServer(socketPath);
    if (!socket.waitForConnected(750)) return {};
    QString requestToken = token;
    if (kind == ServiceKind::Projection) {
        requestToken = astra::common::scopedCapabilityToken(token, astra::common::projectionCapabilityForMethod(method));
    } else if (kind == ServiceKind::SpatialUI && method != QStringLiteral("system.shutdown")) {
        requestToken = astra::common::scopedCapabilityToken(token, astra::common::spatialUICapabilityForMethod(method));
    }
    const QJsonObject request {{QStringLiteral("jsonrpc"), QStringLiteral("2.0")},
                               {QStringLiteral("id"), QUuid::createUuid().toString(QUuid::WithoutBraces)},
                               {QStringLiteral("trace_id"), QUuid::createUuid().toString(QUuid::WithoutBraces)},
                               {QStringLiteral("method"), method}, {QStringLiteral("params"), params},
                               {QStringLiteral("security_context"), QJsonObject {{QStringLiteral("capability_token"), requestToken}}}};
    socket.write(QJsonDocument {request}.toJson(QJsonDocument::Compact) + '\n');
    if (!socket.waitForBytesWritten(750) || !socket.waitForReadyRead(2000)) return {};
    return QJsonDocument::fromJson(socket.readAll().trimmed()).object();
}

bool waitFor(const QString &socketPath, const QString &token, ServiceKind kind)
{
    for (int attempt = 0; attempt < 100; ++attempt) {
        const QString method = kind == ServiceKind::Spatial ? QStringLiteral("spatial.state") : QStringLiteral("system.health");
        const QJsonObject response = rpc(socketPath, method, {}, token, kind);
        if (response.value(QStringLiteral("result")).isObject()) return true;
        QThread::msleep(25);
    }
    return false;
}

void stopProcess(QProcess &process)
{
    if (process.state() == QProcess::NotRunning) return;
    process.terminate();
    if (!process.waitForFinished(2000)) {
        process.kill();
        require(process.waitForFinished(2000), "fixture process stopped");
    }
}

} // namespace

int main(int argc, char **argv)
{
    QCoreApplication application {argc, argv};
    QTemporaryDir temporary;
    require(temporary.isValid(), "temporary directory available");
    const QString root = QString::fromUtf8(ASTRA_TEST_PROJECT_ROOT);
    const QString pid = QString::number(QCoreApplication::applicationPid());
    const QString spatialSocket = QDir::temp().filePath(QStringLiteral("astra-chain-p3-%1.sock").arg(pid));
    const QString projectionSocket = QDir::temp().filePath(QStringLiteral("astra-chain-p4-%1.sock").arg(pid));
    const QString spatialUISocket = QDir::temp().filePath(QStringLiteral("astra-chain-p5-%1.sock").arg(pid));
    QLocalServer::removeServer(spatialSocket);
    QLocalServer::removeServer(projectionSocket);
    QLocalServer::removeServer(spatialUISocket);
    const QString spatialToken = randomToken(QStringLiteral("p3-"));
    const QString projectionToken = randomToken(QStringLiteral("p4-"));
    const QString clientToken = randomToken(QStringLiteral("p5-client-"));
    const QString supervisorToken = randomToken(QStringLiteral("p5-supervisor-"));
    const QString spatialTokenPath = temporary.filePath(QStringLiteral("spatial.token"));
    const QString projectionTokenPath = temporary.filePath(QStringLiteral("projection.token"));
    const QString clientTokenPath = temporary.filePath(QStringLiteral("client.token"));
    const QString supervisorTokenPath = temporary.filePath(QStringLiteral("supervisor.token"));
    writeToken(spatialTokenPath, spatialToken);
    writeToken(projectionTokenPath, projectionToken);
    writeToken(clientTokenPath, clientToken);
    writeToken(supervisorTokenPath, supervisorToken);

    QProcess spatialService;
    QProcess projectionService;
    QProcess spatialUIService;
    spatialService.start(QString::fromUtf8(ASTRA_P3_SERVICE_BINARY),
                         {QStringLiteral("--root"), root, QStringLiteral("--socket"), spatialSocket,
                          QStringLiteral("--token"), spatialToken,
                          QStringLiteral("--capabilities"), QStringLiteral("spatial.read,spatial.control"),
                          QStringLiteral("--fixture"), QDir {root}.filePath(QStringLiteral("assets/spatial-fixtures/desk-front.png"))});
    require(spatialService.waitForStarted(3000), "P3 service started");
    require(waitFor(spatialSocket, spatialToken, ServiceKind::Spatial), "P3 service ready");

    const auto detected = rpc(spatialSocket, QStringLiteral("spatial.detect"), {}, spatialToken, ServiceKind::Spatial)
                              .value(QStringLiteral("result")).toObject();
    require(detected.value(QStringLiteral("surface_candidates")).toInt() > 0, "P3 surface detected");
    const auto selected = rpc(spatialSocket, QStringLiteral("spatial.select"),
                              {{QStringLiteral("manual_confirmation"), false}}, spatialToken, ServiceKind::Spatial)
                              .value(QStringLiteral("result")).toObject();
    require(selected.value(QStringLiteral("selected")).toBool(), "P3 target selected");
    const auto calibrated = rpc(spatialSocket, QStringLiteral("spatial.calibrate"),
                                {{QStringLiteral("corners"), selected.value(QStringLiteral("corners"))}},
                                spatialToken, ServiceKind::Spatial).value(QStringLiteral("result")).toObject();
    const QJsonObject calibratedTarget = calibrated.value(QStringLiteral("target")).toObject();
    require(calibrated.value(QStringLiteral("state")).toString() == QStringLiteral("TRACKING")
                && calibratedTarget.value(QStringLiteral("state")).toString() == QStringLiteral("CALIBRATED"),
            "P3 target calibrated");
    const QString targetId = calibratedTarget.value(QStringLiteral("target_id")).toString();
    require(!QUuid {targetId}.isNull(), "P3 target identity is a UUID");

    projectionService.start(QString::fromUtf8(ASTRA_P4_SERVICE_BINARY),
                            {QStringLiteral("--socket"), projectionSocket,
                             QStringLiteral("--capability-token-file"), projectionTokenPath});
    require(projectionService.waitForStarted(3000), "P4 service started");
    require(waitFor(projectionSocket, projectionToken, ServiceKind::Projection), "P4 service ready");

    spatialUIService.start(QString::fromUtf8(ASTRA_P5_SERVICE_BINARY),
                           {QStringLiteral("--project-root"), root,
                            QStringLiteral("--socket"), spatialUISocket,
                            QStringLiteral("--state"), temporary.filePath(QStringLiteral("ui-state.json")),
                            QStringLiteral("--audit"), temporary.filePath(QStringLiteral("ui-audit.jsonl")),
                            QStringLiteral("--client-token-file"), clientTokenPath,
                            QStringLiteral("--supervisor-token-file"), supervisorTokenPath,
                            QStringLiteral("--projection-socket"), projectionSocket,
                            QStringLiteral("--projection-token-file"), projectionTokenPath,
                            QStringLiteral("--spatial-socket"), spatialSocket,
                            QStringLiteral("--spatial-token-file"), spatialTokenPath});
    require(spatialUIService.waitForStarted(3000), "P5 service started");
    require(waitFor(spatialUISocket, clientToken, ServiceKind::SpatialUI), "P5 service ready with P3 binding");

    const auto task = rpc(
        spatialUISocket, QStringLiteral("spatial_ui.task.create"),
        {{QStringLiteral("schema_version"), QStringLiteral("1.0")},
         {QStringLiteral("task_id"), QStringLiteral("7abf7b1e-dc4f-4b7c-96cc-c3dd628e1ca9")},
         {QStringLiteral("title"), QStringLiteral("P3 target chain")}, {QStringLiteral("summary"), QStringLiteral("Verified service target")},
         {QStringLiteral("intent_type"), QStringLiteral("open_task_surface")}, {QStringLiteral("confidence"), 1.0},
         {QStringLiteral("execution_strategy"), QStringLiteral("fixture_adapter")}, {QStringLiteral("privacy_level"), QStringLiteral("PUBLIC")},
         {QStringLiteral("accessibility_label"), QStringLiteral("P3 target chain: Verified service target")},
         {QStringLiteral("bounds"), QJsonObject {{QStringLiteral("x"), 32.0}, {QStringLiteral("y"), 96.0},
                                                  {QStringLiteral("width"), 360.0}, {QStringLiteral("height"), 180.0}}},
         {QStringLiteral("display_target"), QStringLiteral("BOTH")}},
        clientToken, ServiceKind::SpatialUI);
    if (!task.value(QStringLiteral("result")).toObject().value(QStringLiteral("projection_layer_generated")).toBool()) {
        qCritical().noquote() << QJsonDocument {task}.toJson(QJsonDocument::Compact);
        qCritical().noquote() << spatialUIService.readAllStandardError();
        qCritical().noquote() << projectionService.readAllStandardError();
    }
    require(task.value(QStringLiteral("result")).toObject().value(QStringLiteral("projection_layer_generated")).toBool(),
            "P5 submitted public layer through verified P3 target");
    const auto frame = rpc(projectionSocket, QStringLiteral("projection.output.frame"),
                           {{QStringLiteral("output_id"), QStringLiteral("p4-window-projection")}},
                           projectionToken, ServiceKind::Projection).value(QStringLiteral("result")).toObject();
    require(frame.value(QStringLiteral("p3_integration_status")).toString()
                == QStringLiteral("P3_SERVICE_PROJECTION_TARGET_VERIFIED"),
            "P4 output records verified P3 service target");
    require(frame.value(QStringLiteral("spatial_target_id")).toString() == targetId,
            "P3 target identity reaches P4 output through P5");

    require(rpc(spatialSocket, QStringLiteral("spatial.source.start"),
                {{QStringLiteral("source"), QStringLiteral("SIMULATION")},
                 {QStringLiteral("location"), QDir {root}.filePath(QStringLiteral("assets/spatial-fixtures/no-surface.png"))}},
                spatialToken, ServiceKind::Spatial).contains(QStringLiteral("result")), "P3 loss fixture selected");
    const auto lost = rpc(spatialSocket, QStringLiteral("spatial.detect"), {}, spatialToken, ServiceKind::Spatial)
                          .value(QStringLiteral("result")).toObject();
    require(lost.value(QStringLiteral("state")).toString() == QStringLiteral("LOST"), "P3 target loss observed");
    const auto rejected = rpc(spatialUISocket, QStringLiteral("spatial_ui.layout.reset"), {}, clientToken, ServiceKind::SpatialUI);
    require(rejected.value(QStringLiteral("error")).toObject().value(QStringLiteral("code")).toInt() == 5904,
            "P5 rejects projection after P3 target loss");
    const auto staleFrame = rpc(projectionSocket, QStringLiteral("projection.output.frame"),
                                {{QStringLiteral("output_id"), QStringLiteral("p4-window-projection")}},
                                projectionToken, ServiceKind::Projection);
    require(staleFrame.value(QStringLiteral("error")).toObject().value(QStringLiteral("code")).toInt() == 4018,
            "P3 target loss leaves no stale P4 frame");

    require(rpc(spatialUISocket, QStringLiteral("system.shutdown"), {}, supervisorToken, ServiceKind::SpatialUI)
                .value(QStringLiteral("result")).toObject().value(QStringLiteral("accepted")).toBool(),
            "P5 shutdown accepted");
    require(spatialUIService.waitForFinished(3000), "P5 service stopped");
    static_cast<void>(rpc(projectionSocket, QStringLiteral("projection.session.command"),
                          {{QStringLiteral("session_id"), QUuid::createUuid().toString(QUuid::WithoutBraces)},
                           {QStringLiteral("command"), QStringLiteral("STOP")}},
                          projectionToken, ServiceKind::Projection));
    stopProcess(projectionService);
    stopProcess(spatialService);
    require(!QFileInfo::exists(spatialUISocket), "P5 socket removed");
    QLocalServer::removeServer(projectionSocket);
    QLocalServer::removeServer(spatialSocket);
    QLocalServer::removeServer(spatialUISocket);
}
