#include "astra/projection/service/ProjectionService.h"

#include "astra/policy/ProjectionPolicyService.h"
#include "astra/common/CapabilityToken.h"
#include "astra/common/ErrorCode.h"
#include "astra/common/PrivacyLevel.h"
#include "astra/projection/SafeClearController.h"
#include "astra/render/ProjectionLayer.h"

#include <QBuffer>
#include <QJsonArray>
#include <QJsonDocument>
#include <QLocalServer>
#include <QLocalSocket>
#include <QPainter>
#include <QSet>
#include <QUuid>

#include <optional>
#include <cmath>

namespace astra::projection::service {
namespace {

QString stateName(SessionState state)
{
    switch (state) {
    case SessionState::Idle: return QStringLiteral("IDLE");
    case SessionState::Initializing: return QStringLiteral("INITIALIZING");
    case SessionState::Ready: return QStringLiteral("READY");
    case SessionState::Rendering: return QStringLiteral("RENDERING");
    case SessionState::Paused: return QStringLiteral("PAUSED");
    case SessionState::Clearing: return QStringLiteral("CLEARING");
    case SessionState::Stopping: return QStringLiteral("STOPPING");
    case SessionState::Stopped: return QStringLiteral("STOPPED");
    case SessionState::Error: return QStringLiteral("ERROR");
    }
    return QStringLiteral("ERROR");
}

std::array<astra::render::Point, 4> cornersForFixture(const QString &fixtureId)
{
    using astra::render::Point;
    if (fixtureId == QStringLiteral("left-keystone")) return {Point {28, 0}, Point {228, 0}, Point {255, 143}, Point {0, 143}};
    if (fixtureId == QStringLiteral("right-keystone")) return {Point {0, 0}, Point {228, 0}, Point {255, 143}, Point {28, 143}};
    if (fixtureId == QStringLiteral("top-keystone")) return {Point {0, 20}, Point {255, 20}, Point {255, 143}, Point {0, 143}};
    if (fixtureId == QStringLiteral("bottom-keystone")) return {Point {0, 0}, Point {255, 0}, Point {228, 123}, Point {28, 123}};
    if (fixtureId == QStringLiteral("invalid-self-intersection")) return {Point {0, 0}, Point {255, 143}, Point {0, 143}, Point {255, 0}};
    if (fixtureId == QStringLiteral("invalid-non-invertible")) return {Point {0, 0}, Point {2, 0}, Point {2, 1}, Point {0, 1}};
    return {Point {0, 0}, Point {255, 0}, Point {255, 143}, Point {0, 143}};
}

QImage fixtureFrame(const QString &fixtureId)
{
    QImage frame {256, 144, QImage::Format_RGBA8888};
    if (fixtureId == QStringLiteral("front-rectangle")) {
        for (int y = 0; y < frame.height(); ++y) {
            for (int x = 0; x < frame.width(); ++x) {
                frame.setPixelColor(x, y, ((x / 16) + (y / 16)) % 2 == 0 ? QColor {240, 240, 240} : QColor {24, 24, 24});
            }
        }
        return frame;
    }
    const QColor color = fixtureId == QStringLiteral("left-keystone") ? QColor {0, 180, 255}
        : fixtureId == QStringLiteral("right-keystone") ? QColor {255, 160, 0}
        : QColor {32, 200, 128};
    frame.fill(color);
    return frame;
}

bool hasExactKeys(const QJsonObject &parameters, const QSet<QString> &keys)
{
    if (parameters.size() != keys.size()) return false;
    for (auto iterator = parameters.constBegin(); iterator != parameters.constEnd(); ++iterator) {
        if (!keys.contains(iterator.key())) return false;
    }
    return true;
}

bool isUuid(const QJsonValue &value)
{
    return value.isString() && !QUuid {value.toString()}.isNull();
}

bool isKnownFixture(const QString &fixtureId)
{
    static const QSet<QString> fixtureIds {QStringLiteral("front-rectangle"), QStringLiteral("left-keystone"), QStringLiteral("right-keystone"),
                                            QStringLiteral("top-keystone"), QStringLiteral("bottom-keystone"), QStringLiteral("invalid-self-intersection"),
                                            QStringLiteral("invalid-non-invertible")};
    return fixtureIds.contains(fixtureId);
}

bool isKnownCommand(const QString &command)
{
    static const QSet<QString> commands {QStringLiteral("INITIALIZE"), QStringLiteral("READY"), QStringLiteral("RENDER"), QStringLiteral("PAUSE"),
                                         QStringLiteral("RESUME"), QStringLiteral("CLEAR"), QStringLiteral("STOP")};
    return commands.contains(command);
}

std::optional<SafeClearReason> safeClearReason(const QString &reason)
{
    if (reason == QStringLiteral("NO_PROJECTION")) return SafeClearReason::NoProjection;
    if (reason == QStringLiteral("PRIVATE_SCREEN_ONLY")) return SafeClearReason::PrivateScreenOnly;
    if (reason == QStringLiteral("TARGET_LOST")) return SafeClearReason::TargetLost;
    if (reason == QStringLiteral("HOMOGRAPHY_FAILED")) return SafeClearReason::HomographyFailed;
    if (reason == QStringLiteral("RENDER_PASS_FAILED")) return SafeClearReason::RenderPassFailed;
    if (reason == QStringLiteral("OUTPUT_DISCONNECTED")) return SafeClearReason::OutputDisconnected;
    if (reason == QStringLiteral("SERVICE_EXCEPTION")) return SafeClearReason::ServiceException;
    if (reason == QStringLiteral("MANUAL_STOP")) return SafeClearReason::ManualStop;
    return std::nullopt;
}

bool hasValidRenderParameters(const QJsonObject &params)
{
    if (!hasExactKeys(params, {QStringLiteral("request_id"), QStringLiteral("trace_id"), QStringLiteral("session_id"), QStringLiteral("output_id"),
                               QStringLiteral("privacy_level"), QStringLiteral("fixture_id"), QStringLiteral("layer_ids"), QStringLiteral("p3_integration_status")})) {
        return false;
    }
    if (!isUuid(params.value(QStringLiteral("request_id"))) || !isUuid(params.value(QStringLiteral("trace_id"))) || !isUuid(params.value(QStringLiteral("session_id")))) {
        return false;
    }
    if (params.value(QStringLiteral("output_id")).toString() != QStringLiteral("p4-window-projection")
        || !isKnownFixture(params.value(QStringLiteral("fixture_id")).toString())
        || params.value(QStringLiteral("p3_integration_status")).toString() != QString::fromLatin1(kP3FixtureOnlyMarker)
        || !astra::common::privacyLevelFromString(params.value(QStringLiteral("privacy_level")).toString().toStdString())) {
        return false;
    }
    const QJsonArray layerIds = params.value(QStringLiteral("layer_ids")).toArray();
    if (layerIds.isEmpty()) return false;
    QSet<QString> uniqueLayerIds;
    for (const QJsonValue &layerId : layerIds) {
        if (!isUuid(layerId) || uniqueLayerIds.contains(layerId.toString())) return false;
        uniqueLayerIds.insert(layerId.toString());
    }
    return true;
}

std::optional<astra::render::LayerType> layerType(const QString &value)
{
    using astra::render::LayerType;
    if (value == QStringLiteral("BACKGROUND")) return LayerType::Background;
    if (value == QStringLiteral("SCENE_3D")) return LayerType::Scene3D;
    if (value == QStringLiteral("APPLICATION_SURFACE")) return LayerType::ApplicationSurface;
    if (value == QStringLiteral("AI_ASSISTANT")) return LayerType::AiAssistant;
    if (value == QStringLiteral("NOTIFICATION")) return LayerType::Notification;
    if (value == QStringLiteral("PRIVACY_MASK")) return LayerType::PrivacyMask;
    if (value == QStringLiteral("DEBUG_OVERLAY")) return LayerType::DebugOverlay;
    return std::nullopt;
}

std::optional<QRectF> layerBounds(const QJsonObject &value)
{
    if (!hasExactKeys(value, {QStringLiteral("x"), QStringLiteral("y"), QStringLiteral("width"), QStringLiteral("height")})) return std::nullopt;
    if (!value.value(QStringLiteral("x")).isDouble() || !value.value(QStringLiteral("y")).isDouble()
        || !value.value(QStringLiteral("width")).isDouble() || !value.value(QStringLiteral("height")).isDouble()) return std::nullopt;
    const QRectF result {value.value(QStringLiteral("x")).toDouble(), value.value(QStringLiteral("y")).toDouble(),
                         value.value(QStringLiteral("width")).toDouble(), value.value(QStringLiteral("height")).toDouble()};
    return result.isValid() && result.width() > 0.0 && result.height() > 0.0 ? std::optional<QRectF> {result} : std::nullopt;
}

std::optional<QVector<astra::render::ProjectionLayer>> projectionLayers(const QJsonValue &value, const QString &bindingKey)
{
    if (!value.isArray()) return std::nullopt;
    const QJsonArray values = value.toArray();
    if (values.isEmpty() || values.size() > 500) return std::nullopt;
    QVector<astra::render::ProjectionLayer> layers;
    QSet<QString> ids;
    layers.reserve(values.size());
    for (const auto &entry : values) {
        if (!entry.isObject()) return std::nullopt;
        const QJsonObject object = entry.toObject();
        if (!hasExactKeys(object, {QStringLiteral("layer_id"), QStringLiteral("layer_type"), QStringLiteral("privacy_level"),
                                   QStringLiteral("policy_decision_id"), QStringLiteral("policy_subject_id"),
                                   QStringLiteral("visible"), QStringLiteral("z_index"), QStringLiteral("bounds"), QStringLiteral("public_label")})) return std::nullopt;
        if (!isUuid(object.value(QStringLiteral("layer_id"))) || ids.contains(object.value(QStringLiteral("layer_id")).toString())
            || !isUuid(object.value(QStringLiteral("policy_decision_id")))
            || !isUuid(object.value(QStringLiteral("policy_subject_id")))
            || !object.value(QStringLiteral("visible")).isBool() || !object.value(QStringLiteral("z_index")).isDouble()
            || !object.value(QStringLiteral("public_label")).isString() || object.value(QStringLiteral("public_label")).toString().size() > 80) return std::nullopt;
        const auto type = layerType(object.value(QStringLiteral("layer_type")).toString());
        const auto privacy = astra::common::privacyLevelFromString(object.value(QStringLiteral("privacy_level")).toString().toStdString());
        const auto bounds = layerBounds(object.value(QStringLiteral("bounds")).toObject());
        if (!type || !privacy || !bounds
            || object.value(QStringLiteral("policy_subject_id")).toString() != object.value(QStringLiteral("layer_id")).toString()
            || !astra::policy::ProjectionPolicyService::matchesBinding(object.value(QStringLiteral("policy_decision_id")).toString(),
                                                                        object.value(QStringLiteral("policy_subject_id")).toString(),
                                                                        *privacy,
                                                                        true,
                                                                        bindingKey)) return std::nullopt;
        ids.insert(object.value(QStringLiteral("layer_id")).toString());
        layers.append({object.value(QStringLiteral("layer_id")).toString(), *type, object.value(QStringLiteral("z_index")).toInt(),
                       object.value(QStringLiteral("visible")).toBool(), *privacy, *bounds,
                       object.value(QStringLiteral("public_label")).toString(),
                       object.value(QStringLiteral("policy_decision_id")).toString(),
                       object.value(QStringLiteral("policy_subject_id")).toString()});
    }
    return layers;
}

std::optional<QString> verifiedSpatialTargetId(const QJsonValue &value)
{
    if (!value.isObject()) return std::nullopt;
    const QJsonObject envelope = value.toObject();
    if (!hasExactKeys(envelope, {QStringLiteral("service"), QStringLiteral("input_source"),
                                 QStringLiteral("state"), QStringLiteral("target")})
        || envelope.value(QStringLiteral("service")).toString() != QStringLiteral("astra-spatial-service")
        || envelope.value(QStringLiteral("state")).toString() != QStringLiteral("TRACKING")) {
        return std::nullopt;
    }
    static const QSet<QString> inputSources {QStringLiteral("CAMERA"), QStringLiteral("IMAGE"),
                                             QStringLiteral("VIDEO"), QStringLiteral("SIMULATION")};
    if (!inputSources.contains(envelope.value(QStringLiteral("input_source")).toString())) return std::nullopt;

    const QJsonObject target = envelope.value(QStringLiteral("target")).toObject();
    if (!hasExactKeys(target, {QStringLiteral("target_id"), QStringLiteral("surface_id"), QStringLiteral("state"),
                               QStringLiteral("selected"), QStringLiteral("quality"), QStringLiteral("privacy_level"),
                               QStringLiteral("coordinate_system"), QStringLiteral("corners")})
        || !isUuid(target.value(QStringLiteral("target_id"))) || !isUuid(target.value(QStringLiteral("surface_id")))
        || !target.value(QStringLiteral("selected")).toBool()
        || (target.value(QStringLiteral("state")).toString() != QStringLiteral("CALIBRATED")
            && target.value(QStringLiteral("state")).toString() != QStringLiteral("ACTIVE"))
        || target.value(QStringLiteral("quality")).toString() == QStringLiteral("UNUSABLE")
        || !QSet<QString> {QStringLiteral("EXCELLENT"), QStringLiteral("GOOD"), QStringLiteral("FAIR"), QStringLiteral("POOR")}
                .contains(target.value(QStringLiteral("quality")).toString())
        || !astra::common::privacyLevelFromString(target.value(QStringLiteral("privacy_level")).toString().toStdString())
        || target.value(QStringLiteral("coordinate_system")).toString() != QStringLiteral("IMAGE_PIXEL")) {
        return std::nullopt;
    }
    const QJsonArray corners = target.value(QStringLiteral("corners")).toArray();
    if (corners.size() != 4) return std::nullopt;
    for (const auto &cornerValue : corners) {
        const QJsonObject corner = cornerValue.toObject();
        if (!hasExactKeys(corner, {QStringLiteral("x"), QStringLiteral("y"), QStringLiteral("coordinate_system")})
            || !corner.value(QStringLiteral("x")).isDouble() || !corner.value(QStringLiteral("y")).isDouble()
            || !std::isfinite(corner.value(QStringLiteral("x")).toDouble())
            || !std::isfinite(corner.value(QStringLiteral("y")).toDouble())
            || corner.value(QStringLiteral("coordinate_system")).toString() != QStringLiteral("IMAGE_PIXEL")) {
            return std::nullopt;
        }
    }
    return target.value(QStringLiteral("target_id")).toString();
}

bool hasValidLayerSubmitParameters(const QJsonObject &params, const QString &bindingKey)
{
    const QSet<QString> baseKeys {QStringLiteral("request_id"), QStringLiteral("trace_id"), QStringLiteral("session_id"),
                                  QStringLiteral("output_id"), QStringLiteral("fixture_id"), QStringLiteral("layers"),
                                  QStringLiteral("p3_integration_status")};
    const QString marker = params.value(QStringLiteral("p3_integration_status")).toString();
    const bool fixtureOnly = marker == QString::fromLatin1(kP3FixtureOnlyMarker) && hasExactKeys(params, baseKeys);
    const bool serviceTarget = marker == QString::fromLatin1(kP3ServiceTargetVerifiedMarker)
        && hasExactKeys(params, baseKeys | QSet<QString> {QStringLiteral("spatial_target")})
        && verifiedSpatialTargetId(params.value(QStringLiteral("spatial_target"))).has_value();
    return (fixtureOnly || serviceTarget)
        && isUuid(params.value(QStringLiteral("request_id"))) && isUuid(params.value(QStringLiteral("trace_id")))
        && isUuid(params.value(QStringLiteral("session_id")))
        && params.value(QStringLiteral("output_id")).toString() == QStringLiteral("p4-window-projection")
        && params.value(QStringLiteral("fixture_id")).toString() == QStringLiteral("front-rectangle")
        && projectionLayers(params.value(QStringLiteral("layers")), bindingKey).has_value();
}

QImage spatialLayerFrame(const QVector<astra::render::ProjectionLayer> &layers)
{
    QImage frame {256, 144, QImage::Format_RGBA8888};
    frame.fill(QColor {13, 17, 23});
    QPainter painter {&frame};
    painter.setRenderHint(QPainter::Antialiasing);
    for (const auto &layer : astra::render::compositionOrder(layers)) {
        if (!layer.visible) continue;
        const QRectF scaled {layer.bounds.x() / 5.0, layer.bounds.y() / 5.0,
                             layer.bounds.width() / 5.0, layer.bounds.height() / 5.0};
        QColor fill {55, 113, 168};
        if (layer.type == astra::render::LayerType::Notification) fill = QColor {29, 138, 91};
        if (layer.type == astra::render::LayerType::AiAssistant) fill = QColor {126, 87, 194};
        if (layer.type == astra::render::LayerType::DebugOverlay) fill = QColor {176, 106, 28};
        painter.setPen(QPen {QColor {226, 232, 240}, 1.0});
        painter.setBrush(fill);
        painter.drawRoundedRect(scaled, 3.0, 3.0);
        if (!layer.publicLabel.isEmpty()) {
            QFont font = painter.font();
            font.setPixelSize(8);
            font.setBold(true);
            painter.setFont(font);
            painter.setPen(Qt::white);
            painter.drawText(scaled.adjusted(4.0, 3.0, -4.0, -3.0),
                             Qt::AlignLeft | Qt::AlignTop | Qt::TextWordWrap,
                             layer.publicLabel);
        }
    }
    return frame;
}

} // namespace

ProjectionService::ProjectionService(QString capabilityToken)
    : session_(QUuid::createUuid().toString(QUuid::WithoutBraces))
    , runtime_(output_, session_)
    , capabilityToken_(std::move(capabilityToken))
{
}

ProjectionService::~ProjectionService() { close(); }

bool ProjectionService::listen(const QString &socketName, QString *errorMessage)
{
    close();
    QLocalServer::removeServer(socketName);
    server_ = std::make_unique<QLocalServer>();
    if (!server_->listen(socketName)) {
        if (errorMessage != nullptr) *errorMessage = server_->errorString();
        server_.reset();
        return false;
    }
    socketName_ = socketName;
    QObject::connect(server_.get(), &QLocalServer::newConnection, server_.get(), [this] {
        while (server_->hasPendingConnections()) {
            QLocalSocket *socket = server_->nextPendingConnection();
            QObject::connect(socket, &QLocalSocket::readyRead, socket, [this, socket] {
                while (socket->canReadLine()) {
                    const QByteArray line = socket->readLine().trimmed();
                    if (line.isEmpty()) continue;
                    const QJsonDocument document = QJsonDocument::fromJson(line);
                    const QJsonObject response = document.isObject()
                        ? handleRequest(document.object())
                        : failure({}, 9001, QStringLiteral("Invalid JSON-RPC request"));
                    socket->write(QJsonDocument(response).toJson(QJsonDocument::Compact) + '\n');
                    socket->flush();
                }
            });
            QObject::connect(socket, &QLocalSocket::disconnected, socket, &QObject::deleteLater);
        }
    });
    return true;
}

void ProjectionService::close()
{
    if (server_) server_->close();
    if (!socketName_.isEmpty()) QLocalServer::removeServer(socketName_);
    socketName_.clear();
    server_.reset();
}

QJsonObject ProjectionService::handleRequest(const QJsonObject &request)
{
    const QString requestId = request.value(QStringLiteral("id")).toString();
    if (request.value(QStringLiteral("jsonrpc")).toString() != QStringLiteral("2.0") || requestId.isEmpty()) {
        return failure(requestId, 9001, QStringLiteral("Invalid JSON-RPC request"));
    }
    if (!authorized(request)) return failure(requestId, 5002, QStringLiteral("Capability token is invalid"));
    const QString method = request.value(QStringLiteral("method")).toString();
    const QJsonObject params = request.value(QStringLiteral("params")).toObject();
    if (method == QStringLiteral("system.health")) {
        if (!params.isEmpty()) return failure(requestId, 9001, QStringLiteral("Invalid system.health parameters"));
        return success(requestId, {{QStringLiteral("service"), QStringLiteral("astra-projection-service")},
                                   {QStringLiteral("status"), QStringLiteral("HEALTHY")},
                                   {QStringLiteral("protocol_version"), QStringLiteral("1.0")}});
    }
    if (method == QStringLiteral("projection.session.command")) {
        if (!hasExactKeys(params, {QStringLiteral("session_id"), QStringLiteral("command")}) || !isUuid(params.value(QStringLiteral("session_id")))
            || !isKnownCommand(params.value(QStringLiteral("command")).toString())) {
            return failure(requestId, 9001, QStringLiteral("Invalid projection.session.command parameters"));
        }
        return handleCommand(requestId, params);
    }
    if (method == QStringLiteral("projection.render")) {
        if (!hasValidRenderParameters(params)) {
            return failure(requestId, 9001, QStringLiteral("Invalid projection.render parameters"));
        }
        return handleRender(requestId, params);
    }
    if (method == QStringLiteral("projection.layers.submit")) {
        if (!hasValidLayerSubmitParameters(params, capabilityToken_)) {
            static_cast<void>(SafeClearController::clear(output_, session_, SafeClearReason::ServiceException));
            lastPublicLabels_.clear();
            lastP3IntegrationStatus_ = QString::fromLatin1(kP3FixtureOnlyMarker);
            lastSpatialTargetId_.clear();
            return failure(requestId, 9001, QStringLiteral("Invalid projection.layers.submit parameters"));
        }
        return handleLayerSubmit(requestId, params);
    }
    if (method == QStringLiteral("projection.output.clear")) {
        if (!hasExactKeys(params, {QStringLiteral("session_id"), QStringLiteral("output_id"), QStringLiteral("reason")})
            || !isUuid(params.value(QStringLiteral("session_id"))) || params.value(QStringLiteral("output_id")).toString() != output_.outputId()) {
            return failure(requestId, 9001, QStringLiteral("Invalid projection.output.clear parameters"));
        }
        const auto reason = safeClearReason(params.value(QStringLiteral("reason")).toString());
        if (!reason.has_value()) return failure(requestId, 9001, QStringLiteral("Invalid projection.output.clear reason"));
        const auto result = SafeClearController::clear(output_, session_, *reason);
        if (!result.ok) return failure(requestId, result.errorCode, QStringLiteral("Projection safe clear failed"));
        lastPublicLabels_.clear();
        return success(requestId, {{QStringLiteral("output_id"), output_.outputId()}, {QStringLiteral("state"), QStringLiteral("CLEARED")}, {QStringLiteral("cleared"), true}});
    }
    if (method == QStringLiteral("projection.output.frame")) {
        if (!hasExactKeys(params, {QStringLiteral("output_id")})) return failure(requestId, 9001, QStringLiteral("Invalid projection.output.frame parameters"));
        if (params.value(QStringLiteral("output_id")).toString() != output_.outputId() || lastFrameId_ == 0 || !outputHasContent()) {
            return failure(requestId, 4018, QStringLiteral("Projection output frame is unavailable"));
        }
        QBuffer encoded;
        encoded.open(QIODevice::WriteOnly);
        if (!output_.frame().save(&encoded, "PNG")) return failure(requestId, 4018, QStringLiteral("Projection output frame encoding failed"));
        QJsonObject result {{QStringLiteral("output_id"), output_.outputId()}, {QStringLiteral("frame_id"), static_cast<qint64>(lastFrameId_)},
                            {QStringLiteral("frame_png_base64"), QString::fromLatin1(encoded.data().toBase64())},
                            {QStringLiteral("source"), lastSource_},
                            {QStringLiteral("public_labels"), QJsonArray::fromStringList(lastPublicLabels_)},
                            {QStringLiteral("p3_integration_status"), lastP3IntegrationStatus_}};
        if (!lastSpatialTargetId_.isEmpty()) result.insert(QStringLiteral("spatial_target_id"), lastSpatialTargetId_);
        return success(requestId, result);
    }
    return failure(requestId, 9001, QStringLiteral("Unknown JSON-RPC method"));
}

QString ProjectionService::sessionId() const { return session_.sessionId(); }

bool ProjectionService::outputHasContent() const
{
    const QImage image = output_.frame();
    if (image.isNull()) return false;
    for (int y = 0; y < image.height(); ++y) {
        for (int x = 0; x < image.width(); ++x) {
            if (image.pixelColor(x, y) != QColor(Qt::black)) return true;
        }
    }
    return false;
}

QJsonObject ProjectionService::success(const QString &requestId, const QJsonObject &result) const
{
    return {{QStringLiteral("jsonrpc"), QStringLiteral("2.0")}, {QStringLiteral("id"), requestId}, {QStringLiteral("result"), result}};
}

QJsonObject ProjectionService::failure(const QString &requestId, int code, const QString &message) const
{
    return {{QStringLiteral("jsonrpc"), QStringLiteral("2.0")}, {QStringLiteral("id"), requestId},
            {QStringLiteral("error"), QJsonObject {{QStringLiteral("code"), code}, {QStringLiteral("message"), message},
                                                    {QStringLiteral("data"), QJsonObject {{QStringLiteral("p3_integration_status"), QString::fromLatin1(kP3FixtureOnlyMarker)}}}}}};
}

QJsonObject ProjectionService::handleCommand(const QString &requestId, const QJsonObject &params)
{
    const QString command = params.value(QStringLiteral("command")).toString();
    SessionTransitionResult transition;
    if (command == QStringLiteral("INITIALIZE")) {
        if (session_.state() == SessionState::Stopped) {
            const auto reset = session_.reset();
            if (!reset.ok) return failure(requestId, reset.errorCode, QStringLiteral("Projection session reset failed"));
        }
        if (output_.state() == astra::render::OutputState::Uninitialized || output_.state() == astra::render::OutputState::Stopped) {
            const auto initialized = output_.initialize({256, 144, 60.0});
            if (!initialized.ok) return failure(requestId, initialized.errorCode, initialized.message);
        }
        transition = session_.initialize();
    } else if (command == QStringLiteral("READY")) {
        transition = session_.ready();
    } else if (command == QStringLiteral("PAUSE")) {
        transition = session_.pause();
    } else if (command == QStringLiteral("RESUME")) {
        transition = session_.resume();
    } else if (command == QStringLiteral("CLEAR")) {
        const auto result = SafeClearController::clear(output_, session_, SafeClearReason::ManualStop);
        lastPublicLabels_.clear();
        if (!result.ok) return failure(requestId, result.errorCode, QStringLiteral("Projection safe clear failed"));
        return success(requestId, {{QStringLiteral("session_id"), session_.sessionId()}, {QStringLiteral("state"), stateName(session_.state())}});
    } else if (command == QStringLiteral("STOP")) {
        if (output_.state() == astra::render::OutputState::Uninitialized && session_.state() == SessionState::Idle) {
            return success(requestId, {{QStringLiteral("session_id"), session_.sessionId()}, {QStringLiteral("state"), QStringLiteral("IDLE")}});
        }
        const auto result = SafeClearController::clear(output_, session_, SafeClearReason::ManualStop);
        lastPublicLabels_.clear();
        if (!result.ok) return failure(requestId, result.errorCode, QStringLiteral("Projection safe clear failed"));
        return success(requestId, {{QStringLiteral("session_id"), session_.sessionId()}, {QStringLiteral("state"), stateName(session_.state())}});
    } else {
        return failure(requestId, 4004, QStringLiteral("Invalid projection command"));
    }
    if (!transition.ok) return failure(requestId, transition.errorCode, QStringLiteral("Invalid projection state transition"));
    return success(requestId, {{QStringLiteral("session_id"), session_.sessionId()}, {QStringLiteral("state"), stateName(transition.state)}});
}

QJsonObject ProjectionService::handleRender(const QString &requestId, const QJsonObject &params)
{
    if (params.value(QStringLiteral("p3_integration_status")).toString() != QString::fromLatin1(kP3FixtureOnlyMarker)) {
        SafeClearController::clear(output_, session_, SafeClearReason::ServiceException);
        lastPublicLabels_.clear();
        return failure(requestId, 4010, QStringLiteral("P3 fixture-only marker is required"));
    }
    const QString fixtureId = params.value(QStringLiteral("fixture_id")).toString();
    const auto privacy = astra::common::privacyLevelFromString(params.value(QStringLiteral("privacy_level")).toString().toStdString());
    if (!privacy) return failure(requestId, 4010, QStringLiteral("Invalid projection privacy level"));
    ProjectionRenderRequest request;
    request.input = fixtureFrame(fixtureId);
    request.privacyLevel = *privacy;
    request.layers = {{QStringLiteral("fixture-scene"), astra::render::LayerType::Scene3D, 0, true,
                       astra::common::PrivacyLevel::Public, {}, {},
                       QStringLiteral("5abf7b1e-dc4f-4b7c-96cc-c3dd628e1ca9"), {}}};
    request.corners = cornersForFixture(fixtureId);
    const auto result = runtime_.render(request);
    if (!result.ok || result.frameId == 0) return failure(requestId, result.errorCode == 0 ? 4010 : result.errorCode, QStringLiteral("Projection rendering was safely cleared"));
    lastFrameId_ = result.frameId;
    lastSource_ = QStringLiteral("P4_FIXTURE");
    lastPublicLabels_.clear();
    lastP3IntegrationStatus_ = QString::fromLatin1(kP3FixtureOnlyMarker);
    lastSpatialTargetId_.clear();
    return success(requestId, {{QStringLiteral("session_id"), session_.sessionId()}, {QStringLiteral("state"), QStringLiteral("RENDERING")},
                               {QStringLiteral("frame_id"), static_cast<qint64>(result.frameId)}});
}

QJsonObject ProjectionService::handleLayerSubmit(const QString &requestId, const QJsonObject &params)
{
    const auto layers = projectionLayers(params.value(QStringLiteral("layers")), capabilityToken_);
    if (!layers) return failure(requestId, 9001, QStringLiteral("Invalid projection layer batch"));
    ProjectionRenderRequest request;
    request.input = spatialLayerFrame(*layers);
    request.privacyLevel = astra::common::PrivacyLevel::Public;
    request.layers = *layers;
    request.corners = cornersForFixture(QStringLiteral("front-rectangle"));
    request.p3IntegrationStatus = params.value(QStringLiteral("p3_integration_status")).toString();
    const auto result = runtime_.render(request);
    if (!result.ok || result.frameId == 0) {
        return failure(requestId, result.errorCode == 0 ? 4010 : result.errorCode,
                       QStringLiteral("Projection layer submission was safely cleared"));
    }
    lastFrameId_ = result.frameId;
    lastSource_ = QStringLiteral("P5_SPATIAL_UI");
    lastP3IntegrationStatus_ = request.p3IntegrationStatus;
    lastSpatialTargetId_ = request.p3IntegrationStatus == QString::fromLatin1(kP3ServiceTargetVerifiedMarker)
        ? verifiedSpatialTargetId(params.value(QStringLiteral("spatial_target"))).value_or(QString {}) : QString {};
    lastPublicLabels_.clear();
    for (const auto &layer : *layers) {
        if (layer.visible && !layer.publicLabel.isEmpty()) lastPublicLabels_.append(layer.publicLabel);
    }
    return success(requestId, {{QStringLiteral("session_id"), session_.sessionId()}, {QStringLiteral("state"), QStringLiteral("RENDERING")},
                               {QStringLiteral("frame_id"), static_cast<qint64>(result.frameId)},
                               {QStringLiteral("accepted_layer_count"), layers->size()},
                               {QStringLiteral("privacy_mask_final"), true}});
}

bool ProjectionService::authorized(const QJsonObject &request) const
{
    const QString capability = astra::common::projectionCapabilityForMethod(request.value(QStringLiteral("method")).toString());
    return !capability.isEmpty()
        && request.value(QStringLiteral("security_context")).toObject().value(QStringLiteral("capability_token")).toString()
            == astra::common::scopedCapabilityToken(capabilityToken_, capability);
}

} // namespace astra::projection::service
