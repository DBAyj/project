#include "transport/SpatialJsonRpcServer.h"

#include "astra/common/ErrorCode.h"
#include "astra/common/Identifiers.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QRegularExpression>
#include <QVector>

namespace astra::spatial {
namespace {

QString coordinateName(CoordinateSystem value) { return QString::fromStdString(toString(value)); }

QJsonArray cornersJson(const std::array<SpatialPoint, 4> &corners)
{
    QJsonArray values;
    for (const auto &corner : corners) {
        values.append(QJsonObject {{QStringLiteral("x"), corner.x},
                                   {QStringLiteral("y"), corner.y},
                                   {QStringLiteral("coordinate_system"), coordinateName(corner.coordinateSystem)}});
    }
    return values;
}

bool validPoint(const QJsonObject &value)
{
    return value.size() == 3 && value.value(QStringLiteral("x")).isDouble() && value.value(QStringLiteral("y")).isDouble()
        && value.value(QStringLiteral("coordinate_system")) == QStringLiteral("IMAGE_PIXEL");
}

bool validVector(const QJsonObject &value)
{
    return value.size() == 3 && value.value(QStringLiteral("x")).isDouble() && value.value(QStringLiteral("y")).isDouble()
        && value.value(QStringLiteral("z")).isDouble();
}

bool containsOnly(const QJsonObject &value, std::initializer_list<const char *> names)
{
    if (value.size() > static_cast<qsizetype>(names.size())) return false;
    for (auto iterator = value.begin(); iterator != value.end(); ++iterator) {
        bool found = false;
        for (const char *name : names) found = found || iterator.key() == QLatin1String(name);
        if (!found) return false;
    }
    return true;
}

QString registeredMessage(int code)
{
    return astra::common::errorMessage(static_cast<astra::common::ErrorCode>(code));
}

} // namespace

SpatialJsonRpcServer::SpatialJsonRpcServer(SpatialService *service,
                                           QString capabilityToken,
                                           QStringList grantedCapabilities,
                                           QObject *parent)
    : QObject(parent), service_(service), capabilityToken_(std::move(capabilityToken)),
      grantedCapabilities_(grantedCapabilities.cbegin(), grantedCapabilities.cend())
{
    connect(&server_, &QLocalServer::newConnection, this, [this] { acceptConnections(); });
}

bool SpatialJsonRpcServer::listen(const QString &socketPath)
{
    QLocalServer::removeServer(socketPath);
    if (!server_.listen(socketPath)) return false;
    socketPath_ = socketPath;
    lastPublishedState_ = stateChangedPayload(service_->snapshot());
    return true;
}

void SpatialJsonRpcServer::close()
{
    QVector<QLocalSocket *> sockets;
    sockets.reserve(static_cast<qsizetype>(buffers_.size()));
    for (const auto &entry : buffers_) sockets.append(entry.first);
    buffers_.clear();
    stateSubscribers_.clear();
    for (QLocalSocket *socket : sockets) socket->disconnectFromServer();
    server_.close();
    if (!socketPath_.isEmpty()) QLocalServer::removeServer(socketPath_);
    socketPath_.clear();
}

void SpatialJsonRpcServer::acceptConnections()
{
    while (server_.hasPendingConnections()) {
        QLocalSocket *socket = server_.nextPendingConnection();
        buffers_.emplace(socket, QByteArray {});
        connect(socket, &QLocalSocket::readyRead, this, [this, socket] { readConnection(socket); });
        connect(socket, &QLocalSocket::disconnected, this, [this, socket] {
            buffers_.erase(socket);
            stateSubscribers_.remove(socket);
            socket->deleteLater();
        });
    }
}

void SpatialJsonRpcServer::readConnection(QLocalSocket *socket)
{
    if (buffers_.empty()) return;
    auto entry = buffers_.find(socket);
    if (entry == buffers_.end()) return;
    QByteArray &buffer = entry->second;
    buffer += socket->readAll();
    while (true) {
        const int newline = buffer.indexOf('\n');
        if (newline < 0) return;
        const QByteArray line = buffer.left(newline);
        buffer.remove(0, newline + 1);
        QJsonParseError parseError;
        const QJsonDocument document = QJsonDocument::fromJson(line, &parseError);
        QJsonObject response;
        if (parseError.error != QJsonParseError::NoError || !document.isObject()) {
            response = errorResponse(QJsonValue::Null, 3504, registeredMessage(3504));
        } else {
            response = dispatch(document.object(), socket);
        }
        socket->write(QJsonDocument(response).toJson(QJsonDocument::Compact) + '\n');
        socket->flush();
        if (response.contains(QStringLiteral("result"))) publishStateChangedIfChanged();
    }
}

QJsonObject SpatialJsonRpcServer::dispatch(const QJsonObject &request, QLocalSocket *socket)
{
    const QJsonValue id = request.value(QStringLiteral("id"));
    if (!validEnvelope(request)) {
        return errorResponse(id, 3504, registeredMessage(3504));
    }
    if (request.value(QStringLiteral("security_context")).toObject().value(QStringLiteral("capability_token")) != capabilityToken_) {
        return errorResponse(id, 5001, QStringLiteral("Permission denied"));
    }
    const QString method = request.value(QStringLiteral("method")).toString();
    const QJsonObject params = request.value(QStringLiteral("params")).toObject();
    const QString capability = requiredCapability(method);
    if (capability.isEmpty() || !grantedCapabilities_.contains(capability)) {
        return errorResponse(id, 5001, registeredMessage(5001));
    }
    if (!validParams(method, params)) return errorResponse(id, 3504, registeredMessage(3504));
    service_->setRequestContext(request.value(QStringLiteral("trace_id")).toString(QString::fromStdString(astra::common::newUuid())), id.toString());
    if (method == QStringLiteral("event.subscribe")) {
        stateSubscribers_.insert(socket);
        return successResponse(id, {{QStringLiteral("subscription_id"), QString::fromStdString(astra::common::newUuid())},
                                    {QStringLiteral("event"), QStringLiteral("spatial.state_changed")}});
    }
    if (method == QStringLiteral("spatial.health")) {
        const auto snapshot = service_->snapshot();
        return successResponse(id, {{QStringLiteral("status"), snapshot.state == SpatialServiceState::Stopped ? QStringLiteral("NOT_READY") : QStringLiteral("READY")},
                                    {QStringLiteral("state"), QString::fromStdString(toString(snapshot.state))}});
    }
    if (method == QStringLiteral("spatial.state")) return successResponse(id, snapshotJson(service_->snapshot()));
    if (method == QStringLiteral("spatial.source.start")) {
        const auto source = frameSourceTypeFromString(params.value(QStringLiteral("source")).toString().toStdString());
        if (!source) return errorResponse(id, 3004, QStringLiteral("Spatial input source is unavailable"));
        const auto result = service_->start(*source, params.value(QStringLiteral("location")).toString());
        return result.ok ? successResponse(id, snapshotJson(service_->snapshot())) : errorResponse(id, result.errorCode, result.message);
    }
    if (method == QStringLiteral("spatial.source.stop")) {
        const auto result = service_->stop();
        return result.ok ? successResponse(id, snapshotJson(service_->snapshot())) : errorResponse(id, result.errorCode, result.message);
    }
    if (method == QStringLiteral("spatial.detect")) {
        const auto result = service_->detect();
        return result.ok ? successResponse(id, snapshotJson(result.value)) : errorResponse(id, result.errorCode, result.message);
    }
    if (method == QStringLiteral("spatial.select")) {
        const auto result = service_->select(params.value(QStringLiteral("manual_confirmation")).toBool(false));
        return result.ok ? successResponse(id, targetJson(result.value)) : errorResponse(id, result.errorCode, result.message);
    }
    if (method == QStringLiteral("spatial.calibrate")) {
        std::array<SpatialPoint, 4> corners;
        const QJsonArray values = params.value(QStringLiteral("corners")).toArray();
        for (qsizetype index = 0; index < values.size(); ++index) {
            const QJsonObject point = values.at(index).toObject();
            corners[static_cast<std::size_t>(index)] = {point.value(QStringLiteral("x")).toDouble(), point.value(QStringLiteral("y")).toDouble(), CoordinateSystem::ImagePixel};
        }
        const auto result = service_->calibrate(corners);
        return result.ok ? successResponse(id, snapshotJson(service_->snapshot())) : errorResponse(id, result.errorCode, result.message);
    }
    if (method == QStringLiteral("spatial.observer.set")) {
        const QJsonObject position = params.value(QStringLiteral("position")).toObject();
        const QJsonObject orientation = params.value(QStringLiteral("orientation")).toObject();
        ObserverPose observer;
        observer.position = {position.value(QStringLiteral("x")).toDouble(), position.value(QStringLiteral("y")).toDouble(), position.value(QStringLiteral("z")).toDouble()};
        observer.orientation = {orientation.value(QStringLiteral("x")).toDouble(), orientation.value(QStringLiteral("y")).toDouble(), orientation.value(QStringLiteral("z")).toDouble()};
        observer.trackingState = "SIMULATED";
        observer.confidence = params.value(QStringLiteral("confidence")).toDouble(1.0);
        const auto result = service_->setObserver(observer);
        return result.ok ? successResponse(id, snapshotJson(service_->snapshot())) : errorResponse(id, result.errorCode, result.message);
    }
    if (method == QStringLiteral("spatial.reset")) {
        const auto result = service_->reset();
        return result.ok ? successResponse(id, snapshotJson(service_->snapshot())) : errorResponse(id, result.errorCode, result.message);
    }
    return errorResponse(id, 3504, registeredMessage(3504));
}

void SpatialJsonRpcServer::publishStateChangedIfChanged()
{
    const auto snapshot = service_->snapshot();
    const QJsonObject payload = stateChangedPayload(snapshot);
    if (payload == lastPublishedState_) return;
    lastPublishedState_ = payload;
    const QJsonObject notification {{QStringLiteral("jsonrpc"), QStringLiteral("2.0")},
                                    {QStringLiteral("method"), QStringLiteral("spatial.state_changed")},
                                    {QStringLiteral("params"), payload}};
    const QByteArray serialized = QJsonDocument(notification).toJson(QJsonDocument::Compact) + '\n';
    for (QLocalSocket *socket : stateSubscribers_) {
        if (socket && socket->state() == QLocalSocket::ConnectedState) {
            socket->write(serialized);
            socket->flush();
        }
    }
}

QJsonObject SpatialJsonRpcServer::stateChangedPayload(const SpatialSnapshot &snapshot)
{
    return {{QStringLiteral("state"), QString::fromStdString(toString(snapshot.state))},
            {QStringLiteral("input_source"), QString::fromStdString(toString(snapshot.inputSource))},
            {QStringLiteral("selected_target_id"), snapshot.target.targetId.empty() ? QJsonValue::Null : QJsonValue(QString::fromStdString(snapshot.target.targetId))},
            {QStringLiteral("error_code"), snapshot.lastErrorCode == 0 ? QJsonValue::Null : QJsonValue(snapshot.lastErrorCode)}};
}

bool SpatialJsonRpcServer::validEnvelope(const QJsonObject &request)
{
    if (!containsOnly(request, {"jsonrpc", "id", "trace_id", "method", "params", "security_context"})
        || request.value(QStringLiteral("jsonrpc")) != QStringLiteral("2.0") || !request.value(QStringLiteral("id")).isString()
        || request.value(QStringLiteral("id")).toString().isEmpty() || !request.value(QStringLiteral("method")).isString()
        || !request.value(QStringLiteral("params")).isObject() || !request.value(QStringLiteral("security_context")).isObject()) {
        return false;
    }
    if (request.contains(QStringLiteral("trace_id")) && (!request.value(QStringLiteral("trace_id")).isString() || request.value(QStringLiteral("trace_id")).toString().isEmpty())) return false;
    const QJsonObject security = request.value(QStringLiteral("security_context")).toObject();
    return containsOnly(security, {"capability_token", "delegation_id"}) && security.value(QStringLiteral("capability_token")).isString()
        && !security.value(QStringLiteral("capability_token")).toString().isEmpty();
}

bool SpatialJsonRpcServer::validParams(const QString &method, const QJsonObject &params)
{
    if (method == QStringLiteral("event.subscribe")) return containsOnly(params, {"event"}) && params.value(QStringLiteral("event")) == QStringLiteral("spatial.state_changed");
    if (method == QStringLiteral("spatial.health") || method == QStringLiteral("spatial.state") || method == QStringLiteral("spatial.source.stop")
        || method == QStringLiteral("spatial.detect") || method == QStringLiteral("spatial.reset")) return params.isEmpty();
    if (method == QStringLiteral("spatial.source.start")) {
        return containsOnly(params, {"source", "location"}) && params.value(QStringLiteral("source")).isString()
            && frameSourceTypeFromString(params.value(QStringLiteral("source")).toString().toStdString()).has_value()
            && (!params.contains(QStringLiteral("location")) || (params.value(QStringLiteral("location")).isString()
                && !params.value(QStringLiteral("location")).toString().isEmpty() && params.value(QStringLiteral("location")).toString().size() <= 1024));
    }
    if (method == QStringLiteral("spatial.select")) {
        return containsOnly(params, {"manual_confirmation"})
            && (!params.contains(QStringLiteral("manual_confirmation")) || params.value(QStringLiteral("manual_confirmation")).isBool());
    }
    if (method == QStringLiteral("spatial.calibrate")) {
        if (!containsOnly(params, {"corners"}) || !params.value(QStringLiteral("corners")).isArray()) return false;
        const QJsonArray corners = params.value(QStringLiteral("corners")).toArray();
        if (corners.size() != 4) return false;
        for (const QJsonValue &value : corners) if (!value.isObject() || !validPoint(value.toObject())) return false;
        return true;
    }
    if (method == QStringLiteral("spatial.observer.set")) {
        return containsOnly(params, {"position", "orientation", "confidence"}) && params.value(QStringLiteral("position")).isObject()
            && params.value(QStringLiteral("orientation")).isObject() && params.value(QStringLiteral("confidence")).isDouble()
            && validVector(params.value(QStringLiteral("position")).toObject()) && validVector(params.value(QStringLiteral("orientation")).toObject())
            && params.value(QStringLiteral("confidence")).toDouble() >= 0.0 && params.value(QStringLiteral("confidence")).toDouble() <= 1.0;
    }
    return false;
}

QString SpatialJsonRpcServer::requiredCapability(const QString &method)
{
    if (method == QStringLiteral("event.subscribe")) return QStringLiteral("spatial.read");
    if (method == QStringLiteral("spatial.health") || method == QStringLiteral("spatial.state")) return QStringLiteral("spatial.read");
    if (method == QStringLiteral("spatial.observer.set")) return QStringLiteral("spatial.observer.write");
    if (method.startsWith(QStringLiteral("spatial."))) return QStringLiteral("spatial.control");
    return {};
}

QJsonObject SpatialJsonRpcServer::snapshotJson(const SpatialSnapshot &snapshot)
{
    return {{QStringLiteral("state"), QString::fromStdString(toString(snapshot.state))},
            {QStringLiteral("input_source"), QString::fromStdString(toString(snapshot.inputSource))},
            {QStringLiteral("surface_candidates"), snapshot.candidates.size()},
            {QStringLiteral("scene_id"), snapshot.sceneId},
            {QStringLiteral("target"), targetJson(snapshot.target)},
            {QStringLiteral("quality"), QString::fromStdString(toString(snapshot.target.quality))},
            {QStringLiteral("observer_tracking"), QString::fromStdString(snapshot.observer.trackingState)},
            {QStringLiteral("observer_camera_offset"), QJsonObject {{QStringLiteral("x"), snapshot.observerCameraOffset.x},
                                                                       {QStringLiteral("y"), snapshot.observerCameraOffset.y},
                                                                       {QStringLiteral("z"), snapshot.observerCameraOffset.z}}},
            {QStringLiteral("processing_duration_ms"), snapshot.processingDurationMs},
            {QStringLiteral("last_error_code"), snapshot.lastErrorCode == 0 ? QJsonValue::Null : QJsonValue(snapshot.lastErrorCode)},
            {QStringLiteral("warning"), snapshot.warning}};
}

QJsonObject SpatialJsonRpcServer::targetJson(const ProjectionTarget &target)
{
    return {{QStringLiteral("target_id"), QString::fromStdString(target.targetId)},
            {QStringLiteral("surface_id"), QString::fromStdString(target.surfaceId)},
            {QStringLiteral("state"), QString::fromStdString(toString(target.state))},
            {QStringLiteral("selected"), target.selected},
            {QStringLiteral("quality"), QString::fromStdString(toString(target.quality))},
            {QStringLiteral("privacy_level"), QString::fromStdString(target.privacyLevel)},
            {QStringLiteral("coordinate_system"), coordinateName(target.coordinateSystem)},
            {QStringLiteral("corners"), cornersJson(target.corners)}};
}

QJsonObject SpatialJsonRpcServer::errorResponse(const QJsonValue &id, int code, const QString &message)
{
    return {{QStringLiteral("jsonrpc"), QStringLiteral("2.0")},
            {QStringLiteral("id"), id.isString() ? id : QJsonValue(QString())},
            {QStringLiteral("error"), QJsonObject {{QStringLiteral("code"), code}, {QStringLiteral("message"), message}, {QStringLiteral("data"), QJsonObject {}}}}};
}

QJsonObject SpatialJsonRpcServer::successResponse(const QJsonValue &id, const QJsonObject &result)
{
    return {{QStringLiteral("jsonrpc"), QStringLiteral("2.0")}, {QStringLiteral("id"), id}, {QStringLiteral("result"), result}};
}

} // namespace astra::spatial
