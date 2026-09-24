#include "astra/spatial_ui/service/SpatialUIService.h"

#include "astra/common/Identifiers.h"
#include "astra/common/CapabilityToken.h"
#include "astra/common/PrivacyLevel.h"

#include <QDateTime>
#include <QCryptographicHash>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QHostAddress>
#include <QHash>
#include <QLocalServer>
#include <QLocalSocket>
#include <QTimer>
#include <QTcpServer>
#include <QTcpSocket>
#include <QUuid>
#include <QSet>

namespace astra::spatial_ui::service {
namespace {

bool exactKeys(const QJsonObject &object, const QSet<QString> &keys)
{
    if (object.size() != keys.size()) return false;
    for (auto iterator = object.begin(); iterator != object.end(); ++iterator) {
        if (!keys.contains(iterator.key())) return false;
    }
    return true;
}

bool validUuid(const QJsonValue &value)
{
    return value.isString() && !QUuid {value.toString()}.isNull();
}

QString auditEvent(const QString &method)
{
    if (method == QStringLiteral("spatial_ui.component.create")) return QStringLiteral("component_created");
    if (method == QStringLiteral("spatial_ui.task.create")) return QStringLiteral("task_surface_created");
    if (method == QStringLiteral("spatial_ui.task.update")) return QStringLiteral("task_surface_updated");
    if (method == QStringLiteral("spatial_ui.component.update")) return QStringLiteral("component_shown");
    if (method == QStringLiteral("spatial_ui.component.remove")) return QStringLiteral("component_destroyed");
    if (method == QStringLiteral("spatial_ui.window.open")) return QStringLiteral("window_opened");
    if (method == QStringLiteral("spatial_ui.window.move")) return QStringLiteral("window_moved");
    if (method == QStringLiteral("spatial_ui.window.resize")) return QStringLiteral("window_resized");
    if (method == QStringLiteral("spatial_ui.window.hide")) return QStringLiteral("component_hidden");
    if (method == QStringLiteral("spatial_ui.window.show")) return QStringLiteral("component_shown");
    if (method == QStringLiteral("spatial_ui.window.target")) return QStringLiteral("privacy_visibility_changed");
    if (method == QStringLiteral("spatial_ui.window.restore")) return QStringLiteral("window_moved");
    if (method == QStringLiteral("spatial_ui.window.close")) return QStringLiteral("window_closed");
    if (method == QStringLiteral("spatial_ui.layout.apply")) return QStringLiteral("layout_applied");
    if (method == QStringLiteral("spatial_ui.layout.reset") || method == QStringLiteral("spatial_ui.reset")) return QStringLiteral("layout_reset");
    if (method == QStringLiteral("spatial_ui.input")) return QStringLiteral("input_routed");
    if (method == QStringLiteral("spatial_ui.focus")) return QStringLiteral("focus_changed");
    if (method == QStringLiteral("spatial_ui.notification.create")) return QStringLiteral("notification_created");
    if (method == QStringLiteral("spatial_ui.notification.clear")) return QStringLiteral("notification_expired");
    if (method == QStringLiteral("spatial_ui.state.save")) return QStringLiteral("ui_state_saved");
    if (method == QStringLiteral("spatial_ui.state.load")) return QStringLiteral("ui_state_loaded");
    if (method == QStringLiteral("spatial_ui.target.lost")) return QStringLiteral("privacy_visibility_changed");
    if (method == QStringLiteral("spatial_ui.target.available")) return QStringLiteral("privacy_visibility_changed");
    return {};
}

QJsonObject redactedPayload(const QJsonObject &value)
{
    QJsonObject payload;
    static const QSet<QString> allowed {QStringLiteral("component_id"), QStringLiteral("notification_id"),
                                        QStringLiteral("target_component_id"), QStringLiteral("task_id"), QStringLiteral("window_id"),
                                        QStringLiteral("focus_component_id"), QStringLiteral("projection_safe"),
                                        QStringLiteral("previous_focus_component_id"), QStringLiteral("restored_focus_component_id"),
                                        QStringLiteral("focus_preempted"), QStringLiteral("focus_restored"), QStringLiteral("reason"),
                                        QStringLiteral("projection_frame_id"), QStringLiteral("submitted_layer_count"), QStringLiteral("progress"),
                                        QStringLiteral("interaction_state"), QStringLiteral("interaction_started"),
                                        QStringLiteral("interaction_completed"), QStringLiteral("interaction_cancelled"),
                                        QStringLiteral("notification_cleared"), QStringLiteral("rollback")};
    for (const auto &key : allowed) {
        if (value.contains(key)) payload.insert(key, value.value(key));
    }
    return payload;
}

qsizetype httpContentLength(const QByteArray &header)
{
    for (const auto &line : header.split('\n')) {
        const QByteArray trimmed = line.trimmed();
        if (trimmed.toLower().startsWith("content-length:")) {
            bool ok = false;
            const qlonglong length = trimmed.mid(trimmed.indexOf(':') + 1).trimmed().toLongLong(&ok);
            return ok && length >= 0 ? static_cast<qsizetype>(length) : -1;
        }
    }
    return 0;
}

QString httpMethodFor(const QByteArray &verb, const QByteArray &path)
{
    if (verb == "GET") {
        static const QHash<QByteArray, QString> methods {
            {"/v1/spatial-ui/status", QStringLiteral("spatial_ui.status")},
            {"/v1/spatial-ui/components", QStringLiteral("spatial_ui.components")},
            {"/v1/spatial-ui/windows", QStringLiteral("spatial_ui.windows")},
            {"/v1/spatial-ui/focus", QStringLiteral("spatial_ui.focus.status")},
            {"/v1/spatial-ui/layout", QStringLiteral("spatial_ui.layout.status")},
            {"/v1/spatial-ui/metrics", QStringLiteral("spatial_ui.metrics")},
        };
        return methods.value(path);
    }
    if (verb == "POST") {
        static const QHash<QByteArray, QString> methods {
            {"/v1/spatial-ui/component/create", QStringLiteral("spatial_ui.component.create")},
            {"/v1/spatial-ui/component/update", QStringLiteral("spatial_ui.component.update")},
            {"/v1/spatial-ui/component/remove", QStringLiteral("spatial_ui.component.remove")},
            {"/v1/spatial-ui/task/create", QStringLiteral("spatial_ui.task.create")},
            {"/v1/spatial-ui/task/update", QStringLiteral("spatial_ui.task.update")},
            {"/v1/spatial-ui/window/open", QStringLiteral("spatial_ui.window.open")},
            {"/v1/spatial-ui/window/close", QStringLiteral("spatial_ui.window.close")},
            {"/v1/spatial-ui/window/move", QStringLiteral("spatial_ui.window.move")},
            {"/v1/spatial-ui/window/resize", QStringLiteral("spatial_ui.window.resize")},
            {"/v1/spatial-ui/layout/apply", QStringLiteral("spatial_ui.layout.apply")},
            {"/v1/spatial-ui/layout/reset", QStringLiteral("spatial_ui.layout.reset")},
            {"/v1/spatial-ui/input", QStringLiteral("spatial_ui.input")},
            {"/v1/spatial-ui/interaction/cancel", QStringLiteral("spatial_ui.interaction.cancel")},
            {"/v1/spatial-ui/focus", QStringLiteral("spatial_ui.focus")},
            {"/v1/spatial-ui/notification", QStringLiteral("spatial_ui.notification.create")},
            {"/v1/spatial-ui/reset", QStringLiteral("spatial_ui.reset")},
        };
        return methods.value(path);
    }
    return {};
}

} // namespace

SpatialUIService::SpatialUIService(QString statePath,
                                   QString auditPath,
                                   QString clientToken,
                                   QString supervisorToken,
                                   std::shared_ptr<ProjectionGateway> projectionGateway,
                                   SpatialUIRuntimeOptions options)
    : runtime_(std::move(statePath), std::move(projectionGateway), options)
    , auditPath_(std::move(auditPath))
    , clientToken_(std::move(clientToken))
    , supervisorToken_(std::move(supervisorToken))
    , sessionId_(QString::fromStdString(astra::common::newUuid()))
    , autosaveIntervalMilliseconds_(options.autosaveIntervalSeconds * 1000)
    , statePersistenceEnabled_(options.statePersistence)
{
    maintenanceTimer_.setInterval(50);
    QObject::connect(&maintenanceTimer_, &QTimer::timeout, &maintenanceTimer_, [this] { performMaintenance(); });
}

SpatialUIService::~SpatialUIService() { close(); }

bool SpatialUIService::listen(const QString &socketPath, QString *errorMessage)
{
    if (clientToken_.size() < 32 || supervisorToken_.size() < 32 || clientToken_ == supervisorToken_) {
        if (errorMessage) *errorMessage = QStringLiteral("Spatial UI capability credentials are invalid");
        return false;
    }
    if (server_) {
        if (errorMessage) *errorMessage = QStringLiteral("Service is already listening");
        return false;
    }
    if (QFileInfo::exists(socketPath)) {
        QLocalSocket probe;
        probe.connectToServer(socketPath);
        if (probe.waitForConnected(100)) {
            probe.disconnectFromServer();
            if (errorMessage) *errorMessage = QStringLiteral("Spatial UI service is already running");
            return false;
        }
        QLocalServer::removeServer(socketPath);
    }
    if (!QDir {}.mkpath(QFileInfo {socketPath}.absolutePath())) {
        if (errorMessage) *errorMessage = QStringLiteral("Socket directory creation failed");
        return false;
    }
    server_ = std::make_unique<QLocalServer>();
    if (!server_->listen(socketPath)) {
        if (errorMessage) *errorMessage = server_->errorString();
        server_.reset();
        return false;
    }
    socketPath_ = socketPath;
    QFile::setPermissions(socketPath, QFileDevice::ReadOwner | QFileDevice::WriteOwner);
    QObject::connect(server_.get(), &QLocalServer::newConnection, server_.get(), [this] {
        while (server_->hasPendingConnections()) {
            QLocalSocket *socket = server_->nextPendingConnection();
            QObject::connect(socket, &QLocalSocket::readyRead, socket, [this, socket] {
                while (socket->canReadLine()) {
                    const QJsonDocument document = QJsonDocument::fromJson(socket->readLine().trimmed());
                    const QJsonObject response = document.isObject() ? handleRequest(document.object())
                                                                       : failure({}, 5905, QStringLiteral("Invalid JSON-RPC JSON"));
                    socket->write(QJsonDocument {response}.toJson(QJsonDocument::Compact) + '\n');
                    socket->flush();
                }
            });
            QObject::connect(socket, &QLocalSocket::disconnected, socket, &QObject::deleteLater);
        }
    });
    const QString requestId = QString::fromStdString(astra::common::newUuid());
    audit(QStringLiteral("spatial_ui_service_started"), QString::fromStdString(astra::common::newUuid()), requestId,
          QStringLiteral("success"));
    startMaintenance();
    return true;
}

bool SpatialUIService::listenHttp(const QHostAddress &address, quint16 port, QString *errorMessage)
{
    if (httpServer_) {
        if (errorMessage) *errorMessage = QStringLiteral("HTTP server is already listening");
        return false;
    }
    httpServer_ = std::make_unique<QTcpServer>();
    if (!httpServer_->listen(address, port)) {
        if (errorMessage) *errorMessage = httpServer_->errorString();
        httpServer_.reset();
        return false;
    }
    QObject::connect(httpServer_.get(), &QTcpServer::newConnection, httpServer_.get(), [this] {
        while (httpServer_->hasPendingConnections()) {
            QTcpSocket *socket = httpServer_->nextPendingConnection();
            auto buffer = std::make_shared<QByteArray>();
            QObject::connect(socket, &QTcpSocket::readyRead, socket, [this, socket, buffer] {
                buffer->append(socket->readAll());
                const qsizetype headerEnd = buffer->indexOf("\r\n\r\n");
                if (headerEnd < 0) return;
                const qsizetype contentLength = httpContentLength(buffer->left(headerEnd));
                if (contentLength < 0) {
                    writeHttpResponse(socket, 400, {{QStringLiteral("error"), QStringLiteral("invalid_content_length")}});
                    return;
                }
                if (buffer->size() < headerEnd + 4 + contentLength) return;
                handleHttpRequest(socket, *buffer);
            });
            QObject::connect(socket, &QTcpSocket::disconnected, socket, &QObject::deleteLater);
            QObject::connect(socket, &QTcpSocket::disconnected, httpServer_.get(), [this, socket] { eventSubscribers_.remove(socket); });
        }
    });
    startMaintenance();
    return true;
}

void SpatialUIService::handleHttpRequest(QTcpSocket *socket, const QByteArray &request)
{
    const qsizetype headerEnd = request.indexOf("\r\n\r\n");
    const QList<QByteArray> lines = request.left(headerEnd).split('\n');
    if (lines.isEmpty()) {
        writeHttpResponse(socket, 400, {{QStringLiteral("error"), QStringLiteral("invalid_request")}});
        return;
    }
    const QList<QByteArray> requestParts = lines.first().trimmed().split(' ');
    if (requestParts.size() != 3 || requestParts.at(2) != "HTTP/1.1") {
        writeHttpResponse(socket, 400, {{QStringLiteral("error"), QStringLiteral("invalid_request_line")}});
        return;
    }
    const QByteArray verb = requestParts.at(0);
    const QByteArray path = requestParts.at(1);
    QHash<QByteArray, QByteArray> headers;
    for (qsizetype index = 1; index < lines.size(); ++index) {
        const QByteArray line = lines.at(index).trimmed();
        const qsizetype separator = line.indexOf(':');
        if (separator > 0) headers.insert(line.left(separator).toLower(), line.mid(separator + 1).trimmed());
    }
    if (verb == "GET" && (path == "/health" || path == "/ready")) {
        writeHttpResponse(socket, 200,
                          {{QStringLiteral("service"), QStringLiteral("astra-spatial-ui-service")},
                           {QStringLiteral("status"), path == "/ready" ? QStringLiteral("READY") : QStringLiteral("HEALTHY")},
                           {QStringLiteral("p4_release_status"), QString::fromLatin1(kP4ReleaseMarker)}});
        return;
    }
    const QByteArray bearer = headers.value("authorization");
    if (verb == "GET" && path == "/v1/spatial-ui/events") {
        if (bearer != "Bearer " + astra::common::scopedCapabilityToken(clientToken_, QStringLiteral("spatial_ui.read")).toUtf8()) {
            writeHttpResponse(socket, 401, {{QStringLiteral("error"), QStringLiteral("unauthorized")}});
            return;
        }
        const QByteArray key = headers.value("sec-websocket-key");
        if (headers.value("upgrade").toLower() != "websocket"
            || !headers.value("connection").toLower().contains("upgrade")
            || headers.value("sec-websocket-version") != "13" || key.isEmpty()) {
            writeHttpResponse(socket, 400, {{QStringLiteral("error"), QStringLiteral("invalid_websocket_upgrade")}});
            return;
        }
        const QByteArray accept = QCryptographicHash::hash(key + "258EAFA5-E914-47DA-95CA-C5AB0DC85B11",
                                                           QCryptographicHash::Sha1).toBase64();
        socket->write("HTTP/1.1 101 Switching Protocols\r\nUpgrade: websocket\r\nConnection: Upgrade\r\nSec-WebSocket-Accept: "
                      + accept + "\r\n\r\n");
        socket->flush();
        eventSubscribers_.insert(socket);
        return;
    }
    const QString rpcMethod = httpMethodFor(verb, path);
    if (rpcMethod.isEmpty()) {
        writeHttpResponse(socket, 404, {{QStringLiteral("error"), QStringLiteral("not_found")}});
        return;
    }
    if (bearer != "Bearer " + astra::common::scopedCapabilityToken(clientToken_, astra::common::spatialUICapabilityForMethod(rpcMethod)).toUtf8()) {
        writeHttpResponse(socket, 401, {{QStringLiteral("error"), QStringLiteral("unauthorized")}});
        return;
    }
    QJsonObject params;
    if (verb == "POST") {
        QJsonParseError parseError;
        const QJsonDocument document = QJsonDocument::fromJson(request.mid(headerEnd + 4), &parseError);
        if (parseError.error != QJsonParseError::NoError || !document.isObject()) {
            writeHttpResponse(socket, 400, {{QStringLiteral("error"), QStringLiteral("invalid_json")}});
            return;
        }
        params = document.object();
    }
    const QString requestId = QString::fromStdString(astra::common::newUuid());
    const QJsonObject rpcRequest {{QStringLiteral("jsonrpc"), QStringLiteral("2.0")}, {QStringLiteral("id"), requestId},
                                  {QStringLiteral("trace_id"), QString::fromStdString(astra::common::newUuid())},
                                  {QStringLiteral("method"), rpcMethod}, {QStringLiteral("params"), params},
                                  {QStringLiteral("security_context"), QJsonObject {{QStringLiteral("capability_token"),
                                                                                       astra::common::scopedCapabilityToken(clientToken_, astra::common::spatialUICapabilityForMethod(rpcMethod))}}}};
    const QJsonObject response = handleRequest(rpcRequest);
    if (response.contains(QStringLiteral("error"))) {
        const QJsonObject error = response.value(QStringLiteral("error")).toObject();
        writeHttpResponse(socket, 400, {{QStringLiteral("error"), error.value(QStringLiteral("message"))},
                                        {QStringLiteral("error_code"), error.value(QStringLiteral("code"))}});
        return;
    }
    writeHttpResponse(socket, 200, response.value(QStringLiteral("result")).toObject());
}

void SpatialUIService::writeHttpResponse(QTcpSocket *socket, int status, const QJsonObject &payload)
{
    static const QHash<int, QByteArray> reasons {{200, "OK"}, {400, "Bad Request"}, {401, "Unauthorized"}, {404, "Not Found"}};
    const QByteArray body = QJsonDocument {payload}.toJson(QJsonDocument::Compact);
    socket->write("HTTP/1.1 " + QByteArray::number(status) + " " + reasons.value(status, "Error") + "\r\n");
    socket->write("Content-Type: application/json\r\nConnection: close\r\nContent-Length: " + QByteArray::number(body.size()) + "\r\n\r\n" + body);
    socket->disconnectFromHost();
}

void SpatialUIService::publishEvent(const QJsonObject &event)
{
    const QByteArray payload = QJsonDocument {event}.toJson(QJsonDocument::Compact);
    QByteArray frame;
    frame.append(static_cast<char>(0x81));
    if (payload.size() <= 125) {
        frame.append(static_cast<char>(payload.size()));
    } else if (payload.size() <= 65535) {
        frame.append(static_cast<char>(126));
        frame.append(static_cast<char>((payload.size() >> 8) & 0xff));
        frame.append(static_cast<char>(payload.size() & 0xff));
    } else {
        frame.append(static_cast<char>(127));
        const quint64 size = static_cast<quint64>(payload.size());
        for (int shift = 56; shift >= 0; shift -= 8) frame.append(static_cast<char>((size >> shift) & 0xff));
    }
    frame.append(payload);
    for (auto *subscriber : std::as_const(eventSubscribers_)) {
        if (subscriber && subscriber->state() == QAbstractSocket::ConnectedState) subscriber->write(frame);
    }
}

void SpatialUIService::startMaintenance()
{
    if (maintenanceTimer_.isActive()) return;
    lastAutosaveAt_ = QDateTime::currentDateTimeUtc();
    maintenanceTimer_.start();
}

void SpatialUIService::performMaintenance()
{
    const QDateTime now = QDateTime::currentDateTimeUtc();
    const bool autosaveDue = statePersistenceEnabled_
        && lastAutosaveAt_.msecsTo(now) >= autosaveIntervalMilliseconds_;
    const QString traceId = QString::fromStdString(astra::common::newUuid());
    const QString requestId = QString::fromStdString(astra::common::newUuid());
    const auto response = runtime_.performMaintenance(now, traceId, requestId, autosaveDue);
    if (!response.ok) {
        audit(QStringLiteral("spatial_ui_error"), traceId, requestId, QStringLiteral("error"), response.errorCode);
        return;
    }
    if (response.value.value(QStringLiteral("autosaved")).toBool()) {
        lastAutosaveAt_ = now;
        audit(QStringLiteral("ui_state_saved"), traceId, requestId, QStringLiteral("success"));
    }
    for (const auto &expiredValue : response.value.value(QStringLiteral("expired_notifications")).toArray()) {
        const QJsonObject expired = expiredValue.toObject();
        const QString privacyLevel = expired.value(QStringLiteral("privacy_level")).toString(QStringLiteral("PRIVATE_SCREEN_ONLY"));
        QJsonObject payload {{QStringLiteral("notification_id"), expired.value(QStringLiteral("notification_id"))},
                             {QStringLiteral("component_id"), expired.value(QStringLiteral("component_id"))}};
        if (expired.value(QStringLiteral("focus_restored")).toBool()) {
            payload.insert(QStringLiteral("focus_restored"), true);
            payload.insert(QStringLiteral("restored_focus_component_id"), expired.value(QStringLiteral("restored_focus_component_id")));
            payload.insert(QStringLiteral("reason"), expired.value(QStringLiteral("reason")));
        }
        audit(QStringLiteral("component_removed"), traceId, requestId, QStringLiteral("success"), 0, privacyLevel, payload);
        audit(QStringLiteral("component_destroyed"), traceId, requestId, QStringLiteral("success"), 0, privacyLevel, payload);
        audit(QStringLiteral("notification_expired"), traceId, requestId, QStringLiteral("success"), 0, privacyLevel, payload);
        if (expired.value(QStringLiteral("focus_restored")).toBool()) {
            audit(QStringLiteral("focus_changed"), traceId, requestId, QStringLiteral("success"), 0, privacyLevel, payload);
        }
    }
    if (response.value.contains(QStringLiteral("submitted_layer_count"))) {
        audit(QStringLiteral("projection_layers_submitted"), traceId, requestId, QStringLiteral("success"), 0,
              QStringLiteral("PRIVATE_SCREEN_ONLY"), redactedPayload(response.value));
    }
}

void SpatialUIService::close()
{
    maintenanceTimer_.stop();
    if ((server_ || httpServer_) && !stopAudited_) {
        const QString requestId = QString::fromStdString(astra::common::newUuid());
        audit(QStringLiteral("spatial_ui_service_stopped"), QString::fromStdString(astra::common::newUuid()), requestId,
              QStringLiteral("success"));
        stopAudited_ = true;
    }
    if (server_) server_->close();
    if (server_ && !socketPath_.isEmpty()) QLocalServer::removeServer(socketPath_);
    server_.reset();
    if (httpServer_) httpServer_->close();
    for (auto *subscriber : std::as_const(eventSubscribers_)) {
        if (subscriber) subscriber->disconnectFromHost();
    }
    eventSubscribers_.clear();
    httpServer_.reset();
    socketPath_.clear();
    lastAutosaveAt_ = {};
}

QJsonObject SpatialUIService::handleRequest(const QJsonObject &request)
{
    const QString id = request.value(QStringLiteral("id")).toString();
    const QString traceId = request.value(QStringLiteral("trace_id")).toString();
    const QSet<QString> requestKeys {QStringLiteral("jsonrpc"), QStringLiteral("id"), QStringLiteral("trace_id"),
                                     QStringLiteral("method"), QStringLiteral("params"), QStringLiteral("security_context")};
    if (!exactKeys(request, requestKeys) || request.value(QStringLiteral("jsonrpc")).toString() != QStringLiteral("2.0")
        || !validUuid(request.value(QStringLiteral("id"))) || !validUuid(request.value(QStringLiteral("trace_id")))
        || !request.value(QStringLiteral("method")).isString() || !request.value(QStringLiteral("params")).isObject()
        || !exactKeys(request.value(QStringLiteral("security_context")).toObject(), {QStringLiteral("capability_token")})) {
        return failure(id, 5905, QStringLiteral("Invalid JSON-RPC request"));
    }
    const QString method = request.value(QStringLiteral("method")).toString();
    if (!authorized(request, method)) {
        audit(QStringLiteral("spatial_ui_error"), traceId, id, QStringLiteral("denied"), 5002);
        return failure(id, 5002, QStringLiteral("Capability token is invalid"));
    }
    if (method == QStringLiteral("system.health")) {
        if (!request.value(QStringLiteral("params")).toObject().isEmpty()) return failure(id, 5905, QStringLiteral("Health parameters must be empty"));
        return success(id, {{QStringLiteral("service"), QStringLiteral("astra-spatial-ui-service")}, {QStringLiteral("status"), QStringLiteral("HEALTHY")},
                            {QStringLiteral("protocol_version"), QStringLiteral("1.0")}, {QStringLiteral("p4_release_status"), QString::fromLatin1(kP4ReleaseMarker)}});
    }
    if (method == QStringLiteral("system.shutdown")) {
        if (!request.value(QStringLiteral("params")).toObject().isEmpty()) return failure(id, 5905, QStringLiteral("Shutdown parameters must be empty"));
        audit(QStringLiteral("spatial_ui_service_stopped"), traceId, id, QStringLiteral("success"));
        stopAudited_ = true;
        if (shutdownHandler_) QTimer::singleShot(0, std::move(shutdownHandler_));
        return success(id, {{QStringLiteral("accepted"), true}});
    }
    const QJsonObject params = request.value(QStringLiteral("params")).toObject();
    if (method == QStringLiteral("spatial_ui.input")) {
        audit(QStringLiteral("input_received"), traceId, id, QStringLiteral("success"), 0,
              QStringLiteral("PRIVATE_SCREEN_ONLY"));
    }
    const auto response = runtime_.dispatch(method, params, traceId, id);
    const auto privacy = astra::common::privacyLevelFromString(params.value(QStringLiteral("privacy_level")).toString().toStdString());
    const QString privacyLevel = QString::fromLatin1(astra::common::privacyLevelToString(
        privacy.value_or(astra::common::PrivacyLevel::PrivateScreenOnly)).data());
    QString event = auditEvent(method);
    if (method == QStringLiteral("spatial_ui.component.update")) {
        event = response.value.value(QStringLiteral("visible")).toBool() ? QStringLiteral("component_shown")
                                                                         : QStringLiteral("component_hidden");
    }
    if (!response.ok) {
        if (method == QStringLiteral("spatial_ui.input") && response.errorCode == 5406) {
            audit(QStringLiteral("input_blocked"), traceId, id, QStringLiteral("denied"), response.errorCode,
                  privacyLevel, redactedPayload(response.value));
        }
        audit(QStringLiteral("spatial_ui_error"), traceId, id, QStringLiteral("error"), response.errorCode, privacyLevel);
    } else {
        const QJsonObject payload = redactedPayload(response.value);
        if (method == QStringLiteral("spatial_ui.component.create")) {
            for (const auto &lifecycleEvent : {QStringLiteral("component_created"), QStringLiteral("component_attached"),
                                               QStringLiteral("component_shown")}) {
                audit(lifecycleEvent, traceId, id, QStringLiteral("success"), 0, privacyLevel, payload);
            }
        } else if (method == QStringLiteral("spatial_ui.task.create")) {
            audit(QStringLiteral("task_surface_created"), traceId, id, QStringLiteral("success"), 0, privacyLevel, payload);
            for (const auto &lifecycleEvent : {QStringLiteral("component_created"), QStringLiteral("component_attached"),
                                               QStringLiteral("component_shown")}) {
                audit(lifecycleEvent, traceId, id, QStringLiteral("success"), 0, privacyLevel, payload);
            }
        } else if (method == QStringLiteral("spatial_ui.component.remove")) {
            audit(QStringLiteral("component_removed"), traceId, id, QStringLiteral("success"), 0, privacyLevel, payload);
            audit(QStringLiteral("component_destroyed"), traceId, id, QStringLiteral("success"), 0, privacyLevel, payload);
        } else if (method == QStringLiteral("spatial_ui.notification.create")) {
            audit(QStringLiteral("notification_created"), traceId, id, QStringLiteral("success"), 0, privacyLevel, payload);
            for (const auto &lifecycleEvent : {QStringLiteral("component_created"), QStringLiteral("component_attached"),
                                               QStringLiteral("component_shown")}) {
                audit(lifecycleEvent, traceId, id, QStringLiteral("success"), 0, privacyLevel, payload);
            }
            if (response.value.value(QStringLiteral("focus_preempted")).toBool()) {
                audit(QStringLiteral("focus_preempted"), traceId, id, QStringLiteral("success"), 0, privacyLevel, payload);
            }
        } else if (method == QStringLiteral("spatial_ui.notification.clear")) {
            audit(QStringLiteral("component_removed"), traceId, id, QStringLiteral("success"), 0, privacyLevel, payload);
            audit(QStringLiteral("component_destroyed"), traceId, id, QStringLiteral("success"), 0, privacyLevel, payload);
            audit(QStringLiteral("notification_expired"), traceId, id, QStringLiteral("success"), 0, privacyLevel, payload);
            if (response.value.value(QStringLiteral("focus_restored")).toBool()) {
                audit(QStringLiteral("focus_changed"), traceId, id, QStringLiteral("success"), 0, privacyLevel, payload);
            }
        } else if (method == QStringLiteral("spatial_ui.interaction.cancel")) {
            audit(QStringLiteral("interaction_cancelled"), traceId, id, QStringLiteral("success"), 0, privacyLevel, payload);
        } else if (method == QStringLiteral("spatial_ui.input")) {
            audit(QStringLiteral("input_routed"), traceId, id, QStringLiteral("success"), 0, privacyLevel, payload);
            if (response.value.value(QStringLiteral("notification_cleared")).toBool()) {
                audit(QStringLiteral("component_removed"), traceId, id, QStringLiteral("success"), 0, privacyLevel, payload);
                audit(QStringLiteral("component_destroyed"), traceId, id, QStringLiteral("success"), 0, privacyLevel, payload);
                audit(QStringLiteral("notification_expired"), traceId, id, QStringLiteral("success"), 0, privacyLevel, payload);
                if (response.value.value(QStringLiteral("focus_restored")).toBool()) {
                    audit(QStringLiteral("focus_changed"), traceId, id, QStringLiteral("success"), 0, privacyLevel, payload);
                }
            }
            if (response.value.value(QStringLiteral("interaction_started")).toBool()) {
                audit(QStringLiteral("interaction_started"), traceId, id, QStringLiteral("success"), 0, privacyLevel, payload);
            }
            if (response.value.value(QStringLiteral("interaction_completed")).toBool()) {
                audit(QStringLiteral("interaction_completed"), traceId, id, QStringLiteral("success"), 0, privacyLevel, payload);
            }
        } else if (!event.isEmpty()) {
            audit(event, traceId, id, QStringLiteral("success"), 0, privacyLevel, payload);
        }
        if (response.value.contains(QStringLiteral("submitted_layer_count"))) {
            audit(QStringLiteral("projection_layers_submitted"), traceId, id, QStringLiteral("success"), 0,
                  privacyLevel, redactedPayload(response.value));
        }
    }
    return response.ok ? success(id, response.value) : failure(id, response.errorCode, response.message);
}

QString SpatialUIService::socketPath() const { return socketPath_; }
quint16 SpatialUIService::httpPort() const { return httpServer_ ? httpServer_->serverPort() : 0; }

void SpatialUIService::setShutdownHandler(std::function<void()> handler) { shutdownHandler_ = std::move(handler); }

QJsonObject SpatialUIService::success(const QString &id, const QJsonObject &result) const
{
    return {{QStringLiteral("jsonrpc"), QStringLiteral("2.0")}, {QStringLiteral("id"), id}, {QStringLiteral("result"), result}};
}

QJsonObject SpatialUIService::failure(const QString &id, int code, const QString &message) const
{
    return {{QStringLiteral("jsonrpc"), QStringLiteral("2.0")}, {QStringLiteral("id"), id},
            {QStringLiteral("error"), QJsonObject {{QStringLiteral("code"), code}, {QStringLiteral("message"), message},
                                                    {QStringLiteral("data"), QJsonObject {{QStringLiteral("p4_release_status"), QString::fromLatin1(kP4ReleaseMarker)}}}}}};
}

bool SpatialUIService::authorized(const QJsonObject &request, const QString &method) const
{
    const QString token = request.value(QStringLiteral("security_context")).toObject().value(QStringLiteral("capability_token")).toString();
    if (method == QStringLiteral("system.shutdown")) return token == supervisorToken_;
    const QString capability = astra::common::spatialUICapabilityForMethod(method);
    return !capability.isEmpty() && token == astra::common::scopedCapabilityToken(clientToken_, capability);
}

void SpatialUIService::audit(const QString &event,
                             const QString &traceId,
                             const QString &requestId,
                             const QString &result,
                             int errorCode,
                             const QString &privacyLevel,
                             const QJsonObject &payload)
{
    QDir {}.mkpath(QFileInfo {auditPath_}.absolutePath());
    QFile file {auditPath_};
    if (!file.open(QIODevice::WriteOnly | QIODevice::Append)) return;
    const QJsonObject entry {{QStringLiteral("schema_version"), QStringLiteral("1.0")},
                             {QStringLiteral("event_id"), QString::fromStdString(astra::common::newUuid())}, {QStringLiteral("event"), event},
                             {QStringLiteral("timestamp"), QDateTime::currentDateTimeUtc().toString(Qt::ISODateWithMs)},
                             {QStringLiteral("trace_id"), traceId}, {QStringLiteral("request_id"), requestId},
                             {QStringLiteral("session_id"), sessionId_}, {QStringLiteral("actor"), QStringLiteral("local-shell")},
                             {QStringLiteral("privacy_level"), privacyLevel}, {QStringLiteral("result"), result},
                             {QStringLiteral("error_code"), errorCode == 0 ? QJsonValue {QJsonValue::Null} : QJsonValue {errorCode}},
                             {QStringLiteral("payload"), payload},
                             {QStringLiteral("p4_release_status"), QString::fromLatin1(kP4ReleaseMarker)}};
    file.write(QJsonDocument {entry}.toJson(QJsonDocument::Compact) + '\n');
    publishEvent(entry);
}

} // namespace astra::spatial_ui::service
