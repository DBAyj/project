#include "clients/SpatialUIServiceClient.h"
#include "astra/common/CapabilityToken.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QLocalServer>
#include <QLocalSocket>
#include <QProcess>
#include <QTemporaryDir>
#include <QThread>
#include <QUuid>
#include <QtGlobal>

namespace {

void require(bool condition, const char *message)
{
    if (!condition) qFatal("P5 Shell/service integration requirement failed: %s", message);
}

QString randomToken(const QString &prefix)
{
    return prefix + QUuid::createUuid().toString(QUuid::WithoutBraces) + QUuid::createUuid().toString(QUuid::WithoutBraces);
}

void writeToken(const QString &path, const QString &token)
{
    QFile file {path};
    require(file.open(QIODevice::WriteOnly | QIODevice::Truncate), "open token file");
    require(file.write(token.toUtf8() + '\n') > 0, "write token file");
    file.close();
    require(QFile::setPermissions(path, QFileDevice::ReadOwner | QFileDevice::WriteOwner), "restrict token permissions");
}

QJsonObject rpc(const QString &socketPath, const QString &method, const QJsonObject &params, const QString &token, bool spatial = true)
{
    QLocalSocket socket;
    socket.connectToServer(socketPath);
    if (!socket.waitForConnected(500)) return {};
    QString capability = spatial ? astra::common::spatialUICapabilityForMethod(method)
                                 : astra::common::projectionCapabilityForMethod(method);
    if (method == QStringLiteral("system.shutdown")) capability.clear();
    const QString requestToken = capability.isEmpty() ? token : astra::common::scopedCapabilityToken(token, capability);
    const QJsonObject request {{QStringLiteral("jsonrpc"), QStringLiteral("2.0")},
                               {QStringLiteral("id"), QUuid::createUuid().toString(QUuid::WithoutBraces)},
                               {QStringLiteral("trace_id"), QUuid::createUuid().toString(QUuid::WithoutBraces)},
                               {QStringLiteral("method"), method}, {QStringLiteral("params"), params},
                               {QStringLiteral("security_context"), QJsonObject {{QStringLiteral("capability_token"), requestToken}}}};
    socket.write(QJsonDocument {request}.toJson(QJsonDocument::Compact) + '\n');
    if (!socket.waitForBytesWritten(500) || !socket.waitForReadyRead(1000)) return {};
    return QJsonDocument::fromJson(socket.readAll().trimmed()).object();
}

bool waitForHealth(const QString &socketPath, const QString &token, bool spatial = true)
{
    for (int attempt = 0; attempt < 80; ++attempt) {
        const auto response = rpc(socketPath, QStringLiteral("system.health"), {}, token, spatial);
        if (response.value(QStringLiteral("result")).toObject().value(QStringLiteral("status")).toString() == QStringLiteral("HEALTHY")) return true;
        QThread::msleep(25);
    }
    return false;
}

} // namespace

int main(int argc, char **argv)
{
    QCoreApplication application {argc, argv};
    QTemporaryDir temporary;
    require(temporary.isValid(), "temporary directory");
    const QString projectionSocket = QDir::temp().filePath(QStringLiteral("astra-p5-p4-%1.sock").arg(QCoreApplication::applicationPid()));
    const QString spatialSocket = QDir::temp().filePath(QStringLiteral("astra-p5-ui-%1.sock").arg(QCoreApplication::applicationPid()));
    const QString projectionToken = randomToken(QStringLiteral("p4-"));
    const QString clientToken = randomToken(QStringLiteral("p5-client-"));
    const QString supervisorToken = randomToken(QStringLiteral("p5-supervisor-"));
    const QString projectionTokenPath = temporary.filePath(QStringLiteral("projection.token"));
    const QString clientTokenPath = temporary.filePath(QStringLiteral("client.token"));
    const QString supervisorTokenPath = temporary.filePath(QStringLiteral("supervisor.token"));
    writeToken(projectionTokenPath, projectionToken);
    writeToken(clientTokenPath, clientToken);
    writeToken(supervisorTokenPath, supervisorToken);

    QLocalServer::removeServer(projectionSocket);
    QLocalServer::removeServer(spatialSocket);
    QProcess projectionService;
    projectionService.start(QString::fromUtf8(ASTRA_P4_SERVICE_BINARY),
                            {QStringLiteral("--socket"), projectionSocket,
                             QStringLiteral("--capability-token-file"), projectionTokenPath});
    require(projectionService.waitForStarted(3000), "P4 service started");
    require(waitForHealth(projectionSocket, projectionToken, false), "P4 service healthy");

    QProcess spatialService;
    spatialService.start(QString::fromUtf8(ASTRA_P5_SERVICE_BINARY),
                         {QStringLiteral("--project-root"), QString::fromUtf8(ASTRA_TEST_PROJECT_ROOT),
                          QStringLiteral("--socket"), spatialSocket,
                          QStringLiteral("--state"), temporary.filePath(QStringLiteral("state.json")),
                          QStringLiteral("--audit"), temporary.filePath(QStringLiteral("audit.jsonl")),
                          QStringLiteral("--client-token-file"), clientTokenPath,
                          QStringLiteral("--supervisor-token-file"), supervisorTokenPath,
                          QStringLiteral("--projection-socket"), projectionSocket,
                          QStringLiteral("--projection-token-file"), projectionTokenPath});
    require(spatialService.waitForStarted(3000), "P5 service started");
    require(waitForHealth(spatialSocket, clientToken), "P5 service healthy");

    astra::shell::SpatialUIServiceClient client {spatialSocket, clientToken};
    const auto health = client.status();
    require(health.ok, "P5 status available");
    require(health.result.value(QStringLiteral("p4_release_status")).toString() == QStringLiteral("P4_RELEASE_BASELINE_FINAL"),
            "P4 baseline marker retained");
    bool cycleCountOk = false;
    const int requestedCycles = qEnvironmentVariableIntValue("ASTRA_P5_INTEGRATION_CYCLES", &cycleCountOk);
    const int cycles = cycleCountOk ? requestedCycles : 1;
    require(cycles > 0 && cycles <= 100, "valid integration cycle count");
    for (int cycle = 0; cycle < cycles; ++cycle) {
        const auto publicTask = rpc(
            spatialSocket, QStringLiteral("spatial_ui.task.create"),
            {{QStringLiteral("schema_version"), QStringLiteral("1.0")},
             {QStringLiteral("task_id"), QStringLiteral("7abf7b1e-dc4f-4b7c-96cc-c3dd628e1ca9")},
             {QStringLiteral("title"), QStringLiteral("Public task %1").arg(cycle)},
             {QStringLiteral("summary"), QStringLiteral("Fixture")},
             {QStringLiteral("intent_type"), QStringLiteral("open_task_surface")},
             {QStringLiteral("confidence"), 1.0},
             {QStringLiteral("execution_strategy"), QStringLiteral("fixture_adapter")},
             {QStringLiteral("privacy_level"), QStringLiteral("PUBLIC")},
             {QStringLiteral("accessibility_label"), QStringLiteral("Public task %1: Fixture").arg(cycle)},
             {QStringLiteral("bounds"), QJsonObject {{QStringLiteral("x"), 32.0}, {QStringLiteral("y"), 96.0},
                                                      {QStringLiteral("width"), 360.0}, {QStringLiteral("height"), 180.0}}},
             {QStringLiteral("display_target"), QStringLiteral("BOTH")}},
            clientToken);
        const QJsonObject publicTaskResult = publicTask.value(QStringLiteral("result")).toObject();
        require(publicTaskResult.value(QStringLiteral("projection_layer_generated")).toBool(), "public P5 layer generated");
        auto frame = rpc(projectionSocket, QStringLiteral("projection.output.frame"),
                         {{QStringLiteral("output_id"), QStringLiteral("p4-window-projection")}}, projectionToken, false);
        const QJsonObject publicFrame = frame.value(QStringLiteral("result")).toObject();
        require(publicFrame.value(QStringLiteral("source")).toString() == QStringLiteral("P5_SPATIAL_UI"),
                "P4 reports a P5 spatial UI frame");
        require(publicFrame.value(QStringLiteral("public_labels")).toArray().contains(QStringLiteral("Public task %1: Fixture").arg(cycle)),
                "P4 exposes readable public TaskSurface content");

        const auto privateTask = client.createTaskCard(QStringLiteral("Private task %1").arg(cycle), QStringLiteral("Fixture"), QStringLiteral("PRIVATE_SCREEN_ONLY"));
        require(privateTask.ok && !privateTask.result.value(QStringLiteral("projection_layer_generated")).toBool(), "private P5 layer excluded");
        frame = rpc(projectionSocket, QStringLiteral("projection.output.frame"),
                    {{QStringLiteral("output_id"), QStringLiteral("p4-window-projection")}}, projectionToken, false);
        const QJsonArray publicLabels = frame.value(QStringLiteral("result")).toObject().value(QStringLiteral("public_labels")).toArray();
        require(publicLabels.contains(QStringLiteral("Public task %1: Fixture").arg(cycle)), "public label remains projected");
        require(!publicLabels.contains(QStringLiteral("Private task %1: Fixture").arg(cycle)), "private label never reaches P4 output");
        require(client.status().result.value(QStringLiteral("component_count")).toInt() == 2, "both phone components retained");
        const auto components = client.components();
        require(components.ok, "runtime component tree available to Shell client");
        require(components.result.value(QStringLiteral("components")).toArray().size() == 2,
                "Shell client receives complete runtime component tree");
        require(client.notifyTargetLost().ok, "target loss accepted");
        require(client.status().result.value(QStringLiteral("projection_safe")).toBool(), "P5 enters projection safe state");
        frame = rpc(projectionSocket, QStringLiteral("projection.output.frame"),
                    {{QStringLiteral("output_id"), QStringLiteral("p4-window-projection")}}, projectionToken, false);
        require(frame.value(QStringLiteral("error")).toObject().value(QStringLiteral("code")).toInt() == 4018,
                "P4 exposes no stale frame after target loss");
        require(rpc(spatialSocket, QStringLiteral("spatial_ui.reset"), {}, clientToken).contains(QStringLiteral("result")),
                "P5 cycle reset accepted");
        require(rpc(spatialSocket, QStringLiteral("spatial_ui.target.available"), {}, clientToken).contains(QStringLiteral("result")),
                "fixture target recovery accepted");
    }

    const auto shutdown = rpc(spatialSocket, QStringLiteral("system.shutdown"), {}, supervisorToken);
    require(shutdown.value(QStringLiteral("result")).toObject().value(QStringLiteral("accepted")).toBool(), "P5 supervisor shutdown accepted");
    require(spatialService.waitForFinished(3000), "P5 service stopped");
    require(!QFileInfo::exists(spatialSocket), "P5 socket removed");
    const auto stopped = rpc(projectionSocket, QStringLiteral("projection.session.command"),
                             {{QStringLiteral("session_id"), QUuid::createUuid().toString(QUuid::WithoutBraces)},
                             {QStringLiteral("command"), QStringLiteral("STOP")}}, projectionToken, false);
    require(stopped.contains(QStringLiteral("result")), "P4 stop command accepted");
    projectionService.terminate();
    if (!projectionService.waitForFinished(2000)) {
        projectionService.kill();
        require(projectionService.waitForFinished(2000), "P4 fixture process stopped");
    }
    require(QFileInfo {temporary.filePath(QStringLiteral("audit.jsonl"))}.size() > 0, "P5 audit evidence written");
}
