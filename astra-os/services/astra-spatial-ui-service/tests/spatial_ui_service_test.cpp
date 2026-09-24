#include "astra/spatial_ui/service/SpatialUIService.h"
#include "astra/common/CapabilityToken.h"

#include <QCoreApplication>
#include <QDir>
#include <QElapsedTimer>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QSet>
#include <QHostAddress>
#include <QLocalSocket>
#include <QTcpSocket>
#include <QThread>
#include <QTemporaryDir>
#include <QUuid>
#include <QtGlobal>

using astra::spatial_ui::service::SpatialUIService;

namespace {
void require(bool condition)
{
    static int checkpoint = 0;
    ++checkpoint;
    if (!condition) qFatal("P5 spatial UI service requirement failed at checkpoint %d", checkpoint);
}

constexpr auto kClientToken = "p5-client-token-7abf7b1e-dc4f-4b7c-96cc-c3dd628e1ca9";
constexpr auto kSupervisorToken = "p5-supervisor-token-c38c0e21-96f1-4975-8cf3-fbfaa58c7cda";
constexpr auto kTaskOne = "7abf7b1e-dc4f-4b7c-96cc-c3dd628e1ca9";
constexpr auto kTaskTwo = "c38c0e21-96f1-4975-8cf3-fbfaa58c7cda";

QJsonObject request(const QString &method, QJsonObject params = {}, const QString &token = QString::fromLatin1(kClientToken))
{
    const QString capability = astra::common::spatialUICapabilityForMethod(method);
    const QString scoped = method == QStringLiteral("system.shutdown") ? token : astra::common::scopedCapabilityToken(token, capability);
    return {{QStringLiteral("jsonrpc"), QStringLiteral("2.0")}, {QStringLiteral("id"), QUuid::createUuid().toString(QUuid::WithoutBraces)},
            {QStringLiteral("trace_id"), QUuid::createUuid().toString(QUuid::WithoutBraces)},
            {QStringLiteral("method"), method}, {QStringLiteral("params"), params},
            {QStringLiteral("security_context"), QJsonObject {{QStringLiteral("capability_token"), scoped}}}};
}

struct HttpResponse {
    int status {0};
    QJsonObject body;
};

HttpResponse httpRequest(quint16 port, const QByteArray &requestBytes)
{
    QTcpSocket socket;
    socket.connectToHost(QHostAddress::LocalHost, port);
    require(socket.waitForConnected(2000));
    socket.write(requestBytes);
    require(socket.waitForBytesWritten(2000));
    QElapsedTimer timer;
    timer.start();
    while (socket.state() != QAbstractSocket::UnconnectedState && timer.elapsed() < 2000) {
        QCoreApplication::processEvents();
        QThread::msleep(1);
    }
    const QByteArray response = socket.readAll();
    const int bodyStart = response.indexOf("\r\n\r\n");
    require(bodyStart > 0);
    const QList<QByteArray> statusParts = response.left(response.indexOf("\r\n")).split(' ');
    require(statusParts.size() >= 2);
    return {statusParts.at(1).toInt(), QJsonDocument::fromJson(response.mid(bodyStart + 4)).object()};
}
}

int main(int argc, char **argv)
{
    QCoreApplication app {argc, argv};
    QTemporaryDir temporary;
    require(temporary.isValid());
    const QString auditPath = temporary.filePath(QStringLiteral("audit.jsonl"));
    const QString statePath = temporary.filePath(QStringLiteral("ui-state.json"));
    astra::spatial_ui::service::SpatialUIRuntimeOptions options;
    options.autosaveIntervalSeconds = 1;
    SpatialUIService service {statePath, auditPath, QString::fromLatin1(kClientToken),
                              QString::fromLatin1(kSupervisorToken), {}, options};

    auto response = service.handleRequest(request(QStringLiteral("system.health")));
    require(response.value(QStringLiteral("result")).toObject().value(QStringLiteral("status")).toString() == QStringLiteral("HEALTHY"));
    QJsonObject unauthorized = request(QStringLiteral("spatial_ui.status"));
    unauthorized.insert(QStringLiteral("security_context"),
                        QJsonObject {{QStringLiteral("capability_token"), QStringLiteral("invalid-token")}});
    require(service.handleRequest(unauthorized).value(QStringLiteral("error")).toObject().value(QStringLiteral("code")).toInt() == 5002);
    require(service.handleRequest(request(QStringLiteral("system.shutdown"))).value(QStringLiteral("error")).toObject().value(QStringLiteral("code")).toInt() == 5002);
    const auto supervisorHealth = service.handleRequest(request(QStringLiteral("system.health"), {}, QString::fromLatin1(kSupervisorToken)));
    require(supervisorHealth.value(QStringLiteral("error")).toObject().value(QStringLiteral("code")).toInt() == 5002);

    const QJsonObject createParams {{QStringLiteral("component_id"), QString::fromLatin1(kTaskOne)}, {QStringLiteral("component_type"), QStringLiteral("TASK_CARD")},
                                    {QStringLiteral("privacy_level"), QStringLiteral("PRIVATE_SCREEN_ONLY")}, {QStringLiteral("accessibility_label"), QStringLiteral("Private task")},
                                    {QStringLiteral("bounds"), QJsonObject {{QStringLiteral("x"), 20.0}, {QStringLiteral("y"), 30.0}, {QStringLiteral("width"), 320.0}, {QStringLiteral("height"), 180.0}}},
                                    {QStringLiteral("display_target"), QStringLiteral("BOTH")}};
    response = service.handleRequest(request(QStringLiteral("spatial_ui.component.create"), createParams));
    require(response.contains(QStringLiteral("result")));
    require(!response.value(QStringLiteral("result")).toObject().value(QStringLiteral("projection_layer_generated")).toBool());
    response = service.handleRequest(request(QStringLiteral("spatial_ui.status")));
    require(response.value(QStringLiteral("result")).toObject().value(QStringLiteral("component_count")).toInt() == 1);
    QJsonObject readTokenWrite = request(QStringLiteral("spatial_ui.component.create"), createParams);
    readTokenWrite.insert(QStringLiteral("security_context"),
                          QJsonObject {{QStringLiteral("capability_token"),
                                        astra::common::scopedCapabilityToken(QString::fromLatin1(kClientToken), QStringLiteral("spatial_ui.read"))}});
    require(service.handleRequest(readTokenWrite).value(QStringLiteral("error")).toObject().value(QStringLiteral("code")).toInt() == 5002);

    const QString taskId = QStringLiteral("8f150fe7-bdf6-431e-96b9-ec69202dc0f4");
    response = service.handleRequest(request(
        QStringLiteral("spatial_ui.task.create"),
        {{QStringLiteral("schema_version"), QStringLiteral("1.0")}, {QStringLiteral("task_id"), taskId},
         {QStringLiteral("title"), QStringLiteral("P2 fixture task")}, {QStringLiteral("summary"), QStringLiteral("Create a spatial task surface")},
         {QStringLiteral("intent_type"), QStringLiteral("open_task_surface")}, {QStringLiteral("confidence"), 0.98},
         {QStringLiteral("execution_strategy"), QStringLiteral("fixture_adapter")},
         {QStringLiteral("privacy_level"), QStringLiteral("PRIVATE_SCREEN_ONLY")},
         {QStringLiteral("bounds"), QJsonObject {{QStringLiteral("x"), 24.0}, {QStringLiteral("y"), 220.0},
                                                  {QStringLiteral("width"), 320.0}, {QStringLiteral("height"), 160.0}}},
         {QStringLiteral("display_target"), QStringLiteral("PHONE")},
         {QStringLiteral("accessibility_label"), QStringLiteral("P2 fixture task")}}));
    require(response.value(QStringLiteral("result")).toObject().value(QStringLiteral("task_surface_created")).toBool());
    response = service.handleRequest(request(
        QStringLiteral("spatial_ui.task.update"),
        {{QStringLiteral("schema_version"), QStringLiteral("1.0")}, {QStringLiteral("task_id"), taskId},
         {QStringLiteral("state"), QStringLiteral("READY")}, {QStringLiteral("progress"), 0.25}}));
    require(response.value(QStringLiteral("result")).toObject().value(QStringLiteral("task_surface_updated")).toBool());

    QString error;
    require(service.listenHttp(QHostAddress::LocalHost, 0, &error));
    auto http = httpRequest(service.httpPort(), "GET /v1/spatial-ui/status HTTP/1.1\r\nHost: 127.0.0.1\r\nConnection: close\r\n\r\n");
    require(http.status == 401);
    const QByteArray readAuthorization = "Authorization: Bearer "
        + astra::common::scopedCapabilityToken(QString::fromLatin1(kClientToken), QStringLiteral("spatial_ui.read")).toUtf8() + "\r\n";
    const QByteArray writeAuthorization = "Authorization: Bearer "
        + astra::common::scopedCapabilityToken(QString::fromLatin1(kClientToken), QStringLiteral("spatial_ui.write")).toUtf8() + "\r\n";
    http = httpRequest(service.httpPort(), "GET /v1/spatial-ui/components HTTP/1.1\r\nHost: 127.0.0.1\r\n" + readAuthorization
                                               + "Connection: close\r\n\r\n");
    require(http.status == 200);
    require(http.body.value(QStringLiteral("components")).toArray().size() == 2);
    const QByteArray updateBody = R"({"component_id":"7abf7b1e-dc4f-4b7c-96cc-c3dd628e1ca9","visible":false})";
    http = httpRequest(service.httpPort(), "POST /v1/spatial-ui/component/update HTTP/1.1\r\nHost: 127.0.0.1\r\n" + writeAuthorization
                                               + "Content-Type: application/json\r\nContent-Length: " + QByteArray::number(updateBody.size())
                                               + "\r\nConnection: close\r\n\r\n" + updateBody);
    require(http.status == 200);
    require(!http.body.value(QStringLiteral("visible")).toBool());

    QTcpSocket eventSocket;
    eventSocket.connectToHost(QHostAddress::LocalHost, service.httpPort());
    require(eventSocket.waitForConnected(2000));
    eventSocket.write("GET /v1/spatial-ui/events HTTP/1.1\r\nHost: 127.0.0.1\r\nUpgrade: websocket\r\nConnection: Upgrade\r\n"
                      "Sec-WebSocket-Version: 13\r\nSec-WebSocket-Key: dGhlIHNhbXBsZSBub25jZQ==\r\n" + readAuthorization + "\r\n");
    require(eventSocket.waitForBytesWritten(2000));
    QElapsedTimer eventTimer;
    eventTimer.start();
    while (!eventSocket.canReadLine() && eventTimer.elapsed() < 2000) {
        QCoreApplication::processEvents();
        QThread::msleep(1);
    }
    QByteArray handshake = eventSocket.readAll();
    require(handshake.contains("HTTP/1.1 101 Switching Protocols"));
    require(handshake.contains("Sec-WebSocket-Accept: s3pPLMBiTxaQ9kYGzzhZRbK+xOo="));
    QJsonObject eventComponent = createParams;
    eventComponent.insert(QStringLiteral("component_id"), QString::fromLatin1(kTaskTwo));
    require(service.handleRequest(request(QStringLiteral("spatial_ui.component.create"), eventComponent)).contains(QStringLiteral("result")));
    eventTimer.restart();
    while (eventSocket.bytesAvailable() == 0 && eventTimer.elapsed() < 2000) {
        QCoreApplication::processEvents();
        QThread::msleep(1);
    }
    const QByteArray eventFrame = eventSocket.readAll();
    require(eventFrame.contains("\"event\":\"component_created\""));
    require(eventFrame.contains("P4_RELEASE_BASELINE_FINAL"));
    eventSocket.disconnectFromHost();

    response = service.handleRequest(request(
        QStringLiteral("spatial_ui.focus"),
        {{QStringLiteral("component_id"), QString::fromLatin1(kTaskTwo)},
         {QStringLiteral("reason"), QStringLiteral("service audit baseline")}}));
    require(response.contains(QStringLiteral("result")));
    const QString criticalId = QStringLiteral("9c3bff7e-087a-4872-a52c-8ff3629de1be");
    response = service.handleRequest(request(
        QStringLiteral("spatial_ui.notification.create"),
        {{QStringLiteral("schema_version"), QStringLiteral("1.0")},
         {QStringLiteral("notification_id"), criticalId},
         {QStringLiteral("title"), QStringLiteral("Privacy confirmation")},
         {QStringLiteral("message"), QStringLiteral("Projection authorization requires confirmation")},
         {QStringLiteral("severity"), QStringLiteral("CRITICAL")},
         {QStringLiteral("privacy_level"), QStringLiteral("PRIVATE_SCREEN_ONLY")},
         {QStringLiteral("display_target"), QStringLiteral("PHONE")},
         {QStringLiteral("timeout_ms"), 0}, {QStringLiteral("requires_action"), true},
         {QStringLiteral("actions"), QJsonArray {QStringLiteral("Confirm")}},
         {QStringLiteral("created_at"), QStringLiteral("2026-07-15T12:00:00.000Z")},
         {QStringLiteral("expires_at"), QJsonValue {QJsonValue::Null}}}));
    require(response.value(QStringLiteral("result")).toObject().value(QStringLiteral("focus_preempted")).toBool());
    require(response.value(QStringLiteral("result")).toObject().value(QStringLiteral("previous_focus_component_id")).toString()
            == QString::fromLatin1(kTaskTwo));
    response = service.handleRequest(request(
        QStringLiteral("spatial_ui.input"),
        {{QStringLiteral("schema_version"), QStringLiteral("1.0")},
         {QStringLiteral("event_id"), QStringLiteral("432944a9-2373-4a4b-a899-001f5a3d949d")},
         {QStringLiteral("event_type"), QStringLiteral("AI_ACTION")},
         {QStringLiteral("source_type"), QStringLiteral("AI_INTENT")},
         {QStringLiteral("source_id"), QStringLiteral("notification-action")},
         {QStringLiteral("target_component_id"), criticalId},
         {QStringLiteral("position"), QJsonValue {QJsonValue::Null}},
         {QStringLiteral("action"), QStringLiteral("Confirm")},
         {QStringLiteral("modifiers"), QJsonArray {}},
         {QStringLiteral("timestamp"), QStringLiteral("2026-07-15T12:00:01.000Z")}}));
    const QJsonObject criticalActionResult = response.value(QStringLiteral("result")).toObject();
    require(criticalActionResult.value(QStringLiteral("notification_cleared")).toBool());
    require(criticalActionResult.value(QStringLiteral("component_id")).toString() == criticalId);
    require(criticalActionResult.value(QStringLiteral("notification_id")).toString() == criticalId);
    require(criticalActionResult.value(QStringLiteral("focus_restored")).toBool());
    require(criticalActionResult.value(QStringLiteral("restored_focus_component_id")).toString()
            == QString::fromLatin1(kTaskTwo));

    const QString timedWarningId = QStringLiteral("27342995-e416-410a-adf1-625e8a56152e");
    const QDateTime warningCreatedAt = QDateTime::currentDateTimeUtc();
    response = service.handleRequest(request(
        QStringLiteral("spatial_ui.notification.create"),
        {{QStringLiteral("schema_version"), QStringLiteral("1.0")},
         {QStringLiteral("notification_id"), timedWarningId}, {QStringLiteral("title"), QStringLiteral("Connection")},
         {QStringLiteral("message"), QStringLiteral("Projection target signal is weak")},
         {QStringLiteral("severity"), QStringLiteral("WARNING")}, {QStringLiteral("privacy_level"), QStringLiteral("PRIVATE_SCREEN_ONLY")},
         {QStringLiteral("display_target"), QStringLiteral("PHONE")}, {QStringLiteral("timeout_ms"), 40},
         {QStringLiteral("requires_action"), false}, {QStringLiteral("actions"), QJsonArray {}},
         {QStringLiteral("created_at"), warningCreatedAt.toString(Qt::ISODateWithMs)},
         {QStringLiteral("expires_at"), warningCreatedAt.addMSecs(40).toString(Qt::ISODateWithMs)}}));
    require(response.contains(QStringLiteral("result")));
    QElapsedTimer maintenanceTimer;
    maintenanceTimer.start();
    while (maintenanceTimer.elapsed() < 1150) {
        QCoreApplication::processEvents();
        QThread::msleep(2);
    }
    require(QFileInfo::exists(statePath));
    response = service.handleRequest(request(QStringLiteral("spatial_ui.status")));
    require(response.value(QStringLiteral("result")).toObject().value(QStringLiteral("notification_count")).toInt() == 0);
    require(service.handleRequest(request(
        QStringLiteral("spatial_ui.component.remove"),
        {{QStringLiteral("component_id"), QString::fromLatin1(kTaskTwo)}})).contains(QStringLiteral("result")));

    const QString unownedCriticalId = QStringLiteral("0ae4506e-33ba-477f-932f-7181ea0bc848");
    response = service.handleRequest(request(
        QStringLiteral("spatial_ui.notification.create"),
        {{QStringLiteral("schema_version"), QStringLiteral("1.0")},
         {QStringLiteral("notification_id"), unownedCriticalId},
         {QStringLiteral("title"), QStringLiteral("Standalone confirmation")},
         {QStringLiteral("message"), QStringLiteral("No previous focus owner exists")},
         {QStringLiteral("severity"), QStringLiteral("CRITICAL")},
         {QStringLiteral("privacy_level"), QStringLiteral("PRIVATE_SCREEN_ONLY")},
         {QStringLiteral("display_target"), QStringLiteral("PHONE")},
         {QStringLiteral("timeout_ms"), 0}, {QStringLiteral("requires_action"), true},
         {QStringLiteral("actions"), QJsonArray {QStringLiteral("Acknowledge")}},
         {QStringLiteral("created_at"), QStringLiteral("2026-07-15T12:00:02.000Z")},
         {QStringLiteral("expires_at"), QJsonValue {QJsonValue::Null}}}));
    require(response.value(QStringLiteral("result")).toObject().value(QStringLiteral("previous_focus_component_id")).toString().isEmpty());
    response = service.handleRequest(request(
        QStringLiteral("spatial_ui.input"),
        {{QStringLiteral("schema_version"), QStringLiteral("1.0")},
         {QStringLiteral("event_id"), QStringLiteral("4a0e7122-4261-4671-9722-bc5234da230b")},
         {QStringLiteral("event_type"), QStringLiteral("AI_ACTION")},
         {QStringLiteral("source_type"), QStringLiteral("SYSTEM")},
         {QStringLiteral("source_id"), QStringLiteral("astra-shell")},
         {QStringLiteral("target_component_id"), unownedCriticalId},
         {QStringLiteral("position"), QJsonValue {QJsonValue::Null}},
         {QStringLiteral("action"), QStringLiteral("Acknowledge")},
         {QStringLiteral("modifiers"), QJsonArray {}},
         {QStringLiteral("timestamp"), QStringLiteral("2026-07-15T12:00:03.000Z")}}));
    const QJsonObject unownedCriticalResult = response.value(QStringLiteral("result")).toObject();
    require(unownedCriticalResult.value(QStringLiteral("focus_restored")).toBool());
    require(unownedCriticalResult.value(QStringLiteral("restored_focus_component_id")).toString().isEmpty());

    const QString socketPath = QDir::temp().filePath(QStringLiteral("astra-p5-%1.sock").arg(QCoreApplication::applicationPid()));
    require(service.listen(socketPath, &error));
    QLocalSocket client;
    client.connectToServer(socketPath);
    require(client.waitForConnected(2000));
    client.write(QJsonDocument(request(QStringLiteral("system.health"))).toJson(QJsonDocument::Compact) + '\n');
    require(client.waitForBytesWritten(2000));
    QElapsedTimer timer;
    timer.start();
    while (!client.canReadLine() && timer.elapsed() < 2000) {
        QCoreApplication::processEvents();
        QThread::msleep(1);
    }
    require(client.canReadLine());
    const auto socketResponse = QJsonDocument::fromJson(client.readLine()).object();
    require(socketResponse.value(QStringLiteral("result")).toObject().value(QStringLiteral("service")).toString() == QStringLiteral("astra-spatial-ui-service"));
    client.disconnectFromServer();
    service.close();
    require(!QFileInfo::exists(socketPath));

    QFile auditFile {auditPath};
    require(auditFile.open(QIODevice::ReadOnly));
    int auditedEvents = 0;
    QSet<QString> auditedEventNames;
    bool focusPreemptionReasonPreserved = false;
    bool focusRestoreReasonPreserved = false;
    QSet<QString> taskLifecycleEvents;
    QSet<QString> criticalNotificationLifecycleEvents;
    QSet<QString> timedNotificationExpiryEvents;
    bool emptyFocusRestoreAudited = false;
    while (!auditFile.atEnd()) {
        const QJsonObject event = QJsonDocument::fromJson(auditFile.readLine().trimmed()).object();
        require(!QUuid {event.value(QStringLiteral("event_id")).toString()}.isNull());
        require(!QUuid {event.value(QStringLiteral("trace_id")).toString()}.isNull());
        require(!QUuid {event.value(QStringLiteral("request_id")).toString()}.isNull());
        require(!QUuid {event.value(QStringLiteral("session_id")).toString()}.isNull());
        require(event.value(QStringLiteral("payload")).isObject());
        const QString eventName = event.value(QStringLiteral("event")).toString();
        auditedEventNames.insert(eventName);
        const QJsonObject payload = event.value(QStringLiteral("payload")).toObject();
        const bool hasCriticalIdentity = payload.value(QStringLiteral("notification_id")).toString() == criticalId
            && payload.value(QStringLiteral("component_id")).toString() == criticalId;
        if (hasCriticalIdentity
            && (eventName == QStringLiteral("notification_created")
                || eventName == QStringLiteral("component_created")
                || eventName == QStringLiteral("component_attached")
                || eventName == QStringLiteral("component_shown")
                || eventName == QStringLiteral("component_removed")
                || eventName == QStringLiteral("component_destroyed")
                || eventName == QStringLiteral("notification_expired")
                || eventName == QStringLiteral("focus_changed"))) {
            criticalNotificationLifecycleEvents.insert(eventName);
        }
        const bool hasTimedNotificationIdentity = payload.value(QStringLiteral("notification_id")).toString() == timedWarningId
            && payload.value(QStringLiteral("component_id")).toString() == timedWarningId;
        if (hasTimedNotificationIdentity
            && (eventName == QStringLiteral("component_removed")
                || eventName == QStringLiteral("component_destroyed")
                || eventName == QStringLiteral("notification_expired"))) {
            timedNotificationExpiryEvents.insert(eventName);
        }
        if (payload.value(QStringLiteral("component_id")).toString() == taskId
            && (eventName == QStringLiteral("component_created")
                || eventName == QStringLiteral("component_attached")
                || eventName == QStringLiteral("component_shown"))) {
            taskLifecycleEvents.insert(eventName);
        }
        if (eventName == QStringLiteral("focus_preempted")) {
            focusPreemptionReasonPreserved = focusPreemptionReasonPreserved
                || (payload.value(QStringLiteral("reason")).toString() == QStringLiteral("critical notification")
                    && payload.value(QStringLiteral("previous_focus_component_id")).toString() == QString::fromLatin1(kTaskTwo));
        }
        if (eventName == QStringLiteral("focus_changed")
            && payload.value(QStringLiteral("reason")).toString() == QStringLiteral("critical notification handled")) {
            focusRestoreReasonPreserved = focusRestoreReasonPreserved
                || payload.value(QStringLiteral("restored_focus_component_id")).toString() == QString::fromLatin1(kTaskTwo);
            emptyFocusRestoreAudited = emptyFocusRestoreAudited
                || (payload.value(QStringLiteral("notification_id")).toString() == unownedCriticalId
                    && payload.value(QStringLiteral("component_id")).toString() == unownedCriticalId
                    && payload.value(QStringLiteral("restored_focus_component_id")).toString().isEmpty());
        }
        ++auditedEvents;
    }
    require(auditedEvents >= 15);
    for (const auto &requiredEvent : {QStringLiteral("component_created"), QStringLiteral("component_attached"),
                                      QStringLiteral("component_shown"), QStringLiteral("component_removed"),
                                      QStringLiteral("component_destroyed"), QStringLiteral("focus_preempted"),
                                      QStringLiteral("focus_changed"), QStringLiteral("task_surface_created"),
                                      QStringLiteral("task_surface_updated"), QStringLiteral("notification_expired"),
                                      QStringLiteral("ui_state_saved")}) {
        require(auditedEventNames.contains(requiredEvent));
    }
    require(focusPreemptionReasonPreserved && focusRestoreReasonPreserved);
    require(taskLifecycleEvents == QSet<QString> {QStringLiteral("component_created"),
                                                  QStringLiteral("component_attached"),
                                                  QStringLiteral("component_shown")});
    require(criticalNotificationLifecycleEvents
            == QSet<QString> {QStringLiteral("notification_created"), QStringLiteral("component_created"),
                              QStringLiteral("component_attached"), QStringLiteral("component_shown"),
                              QStringLiteral("component_removed"), QStringLiteral("component_destroyed"),
                              QStringLiteral("notification_expired"), QStringLiteral("focus_changed")});
    require(timedNotificationExpiryEvents
            == QSet<QString> {QStringLiteral("component_removed"), QStringLiteral("component_destroyed"),
                              QStringLiteral("notification_expired")});
    require(emptyFocusRestoreAudited);
}
