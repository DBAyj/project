#include "astra/spatial_ui/service/ProjectionGateway.h"

#include "astra/common/Identifiers.h"
#include "astra/common/CapabilityToken.h"
#include "astra/common/PrivacyLevel.h"
#include "astra/projection/ProjectionRuntime.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QLocalSocket>

namespace astra::spatial_ui::service {
namespace {

QString layerTypeName(astra::render::LayerType type)
{
    using astra::render::LayerType;
    switch (type) {
    case LayerType::Background: return QStringLiteral("BACKGROUND");
    case LayerType::Scene3D: return QStringLiteral("SCENE_3D");
    case LayerType::ApplicationSurface: return QStringLiteral("APPLICATION_SURFACE");
    case LayerType::AiAssistant: return QStringLiteral("AI_ASSISTANT");
    case LayerType::Notification: return QStringLiteral("NOTIFICATION");
    case LayerType::PrivacyMask: return QStringLiteral("PRIVACY_MASK");
    case LayerType::DebugOverlay: return QStringLiteral("DEBUG_OVERLAY");
    }
    return QStringLiteral("APPLICATION_SURFACE");
}

QString uuidOrNew(const QString &value)
{
    return value.isEmpty() ? QString::fromStdString(astra::common::newUuid()) : value;
}

} // namespace

UnixProjectionGateway::UnixProjectionGateway(QString socketPath, QString capabilityToken,
                                             QString spatialSocketPath, QString spatialCapabilityToken)
    : socketPath_(std::move(socketPath))
    , capabilityToken_(std::move(capabilityToken))
    , spatialSocketPath_(std::move(spatialSocketPath))
    , spatialCapabilityToken_(std::move(spatialCapabilityToken))
{
}

ProjectionGatewayResult UnixProjectionGateway::submit(const QVector<astra::render::ProjectionLayer> &layers,
                                                       const QString &traceId,
                                                       const QString &requestId)
{
    if (layers.isEmpty()) return {{true, 0, {}}, 0};
    const auto ready = ensureSession(traceId, requestId);
    if (!ready.ok) return ready;
    QJsonArray encodedLayers;
    for (const auto &layer : layers) {
        encodedLayers.append(QJsonObject {
            {QStringLiteral("layer_id"), layer.id},
            {QStringLiteral("layer_type"), layerTypeName(layer.type)},
            {QStringLiteral("privacy_level"), QString::fromLatin1(astra::common::privacyLevelToString(layer.privacyLevel).data())},
            {QStringLiteral("policy_decision_id"), layer.policyDecisionId},
            {QStringLiteral("policy_subject_id"), layer.policySubjectId},
            {QStringLiteral("visible"), layer.visible},
            {QStringLiteral("z_index"), layer.zIndex},
            {QStringLiteral("bounds"), QJsonObject {{QStringLiteral("x"), layer.bounds.x()}, {QStringLiteral("y"), layer.bounds.y()},
                                                     {QStringLiteral("width"), layer.bounds.width()}, {QStringLiteral("height"), layer.bounds.height()}}},
            {QStringLiteral("public_label"), layer.publicLabel.left(80)},
        });
    }
    QJsonObject params {{QStringLiteral("request_id"), uuidOrNew(requestId)}, {QStringLiteral("trace_id"), uuidOrNew(traceId)},
                        {QStringLiteral("session_id"), sessionId_}, {QStringLiteral("output_id"), QStringLiteral("p4-window-projection")},
                        {QStringLiteral("fixture_id"), QStringLiteral("front-rectangle")}, {QStringLiteral("layers"), encodedLayers},
                        {QStringLiteral("p3_integration_status"), QString::fromLatin1(astra::projection::kP3FixtureOnlyMarker)}};
    if (!spatialSocketPath_.isEmpty() || !spatialCapabilityToken_.isEmpty()) {
        const QJsonObject target = verifiedSpatialTarget(traceId, requestId);
        if (target.isEmpty()) return {{false, 5904, QStringLiteral("P3 projection target is unavailable or invalid")}, 0};
        params.insert(QStringLiteral("p3_integration_status"), QString::fromLatin1(astra::projection::kP3ServiceTargetVerifiedMarker));
        params.insert(QStringLiteral("spatial_target"), target);
    }
    return invoke(QStringLiteral("projection.layers.submit"), params, traceId, requestId);
}

QJsonObject UnixProjectionGateway::verifiedSpatialTarget(const QString &traceId, const QString &requestId) const
{
    if (spatialSocketPath_.isEmpty() || spatialCapabilityToken_.isEmpty()) return {};
    QLocalSocket socket;
    socket.connectToServer(spatialSocketPath_);
    if (!socket.waitForConnected(750)) return {};
    const QJsonObject request {{QStringLiteral("jsonrpc"), QStringLiteral("2.0")},
                               {QStringLiteral("id"), uuidOrNew(requestId)},
                               {QStringLiteral("trace_id"), uuidOrNew(traceId)},
                               {QStringLiteral("method"), QStringLiteral("spatial.state")},
                               {QStringLiteral("params"), QJsonObject {}},
                               {QStringLiteral("security_context"), QJsonObject {{QStringLiteral("capability_token"), spatialCapabilityToken_}}}};
    socket.write(QJsonDocument {request}.toJson(QJsonDocument::Compact) + '\n');
    if (!socket.waitForBytesWritten(750) || !socket.waitForReadyRead(1500)) return {};
    const QJsonObject response = QJsonDocument::fromJson(socket.readAll().trimmed()).object();
    const QJsonObject result = response.value(QStringLiteral("result")).toObject();
    const QJsonObject target = result.value(QStringLiteral("target")).toObject();
    if (result.value(QStringLiteral("state")).toString() != QStringLiteral("TRACKING")
        || (result.value(QStringLiteral("input_source")).toString() != QStringLiteral("CAMERA")
            && result.value(QStringLiteral("input_source")).toString() != QStringLiteral("IMAGE")
            && result.value(QStringLiteral("input_source")).toString() != QStringLiteral("VIDEO")
            && result.value(QStringLiteral("input_source")).toString() != QStringLiteral("SIMULATION"))
        || QUuid {target.value(QStringLiteral("target_id")).toString()}.isNull()
        || QUuid {target.value(QStringLiteral("surface_id")).toString()}.isNull()
        || !target.value(QStringLiteral("selected")).toBool()
        || (target.value(QStringLiteral("state")).toString() != QStringLiteral("CALIBRATED")
            && target.value(QStringLiteral("state")).toString() != QStringLiteral("ACTIVE"))
        || target.value(QStringLiteral("coordinate_system")).toString() != QStringLiteral("IMAGE_PIXEL")
        || target.value(QStringLiteral("corners")).toArray().size() != 4) {
        return {};
    }
    return {{QStringLiteral("service"), QStringLiteral("astra-spatial-service")},
            {QStringLiteral("input_source"), result.value(QStringLiteral("input_source"))},
            {QStringLiteral("state"), result.value(QStringLiteral("state"))},
            {QStringLiteral("target"), target}};
}

astra::ui::OperationResult UnixProjectionGateway::clear(const QString &reason,
                                                         const QString &traceId,
                                                         const QString &requestId)
{
    if (sessionId_.isEmpty()) return {true, 0, {}};
    const auto response = invoke(QStringLiteral("projection.output.clear"),
                                 {{QStringLiteral("session_id"), sessionId_}, {QStringLiteral("output_id"), QStringLiteral("p4-window-projection")},
                                  {QStringLiteral("reason"), reason}}, traceId, requestId);
    if (response.ok) sessionId_.clear();
    return {response.ok, response.errorCode, response.message};
}

ProjectionGatewayResult UnixProjectionGateway::ensureSession(const QString &traceId, const QString &requestId)
{
    if (!sessionId_.isEmpty()) return {{true, 0, {}}, 0};
    sessionId_ = QString::fromStdString(astra::common::newUuid());
    auto result = invoke(QStringLiteral("projection.session.command"),
                         {{QStringLiteral("session_id"), sessionId_}, {QStringLiteral("command"), QStringLiteral("INITIALIZE")}},
                         traceId, requestId);
    if (!result.ok) {
        sessionId_.clear();
        return result;
    }
    result = invoke(QStringLiteral("projection.session.command"),
                    {{QStringLiteral("session_id"), sessionId_}, {QStringLiteral("command"), QStringLiteral("READY")}},
                    traceId, requestId);
    if (!result.ok) sessionId_.clear();
    return result;
}

ProjectionGatewayResult UnixProjectionGateway::invoke(const QString &method,
                                                       const QJsonObject &params,
                                                       const QString &traceId,
                                                       const QString &requestId) const
{
    if (socketPath_.isEmpty() || capabilityToken_.isEmpty()) return {{false, 5904, QStringLiteral("P4 projection gateway is not configured")}, 0};
    const QString capability = astra::common::projectionCapabilityForMethod(method);
    if (capability.isEmpty()) return {{false, 5904, QStringLiteral("P4 projection method has no capability mapping")}, 0};
    QLocalSocket socket;
    socket.connectToServer(socketPath_);
    if (!socket.waitForConnected(750)) return {{false, 5904, QStringLiteral("P4 projection service is unavailable")}, 0};
    const QJsonObject request {{QStringLiteral("jsonrpc"), QStringLiteral("2.0")}, {QStringLiteral("id"), uuidOrNew(requestId)},
                               {QStringLiteral("trace_id"), uuidOrNew(traceId)}, {QStringLiteral("method"), method},
                               {QStringLiteral("params"), params},
                               {QStringLiteral("security_context"), QJsonObject {{QStringLiteral("capability_token"),
                                                                                    astra::common::scopedCapabilityToken(capabilityToken_, capability)}}}};
    socket.write(QJsonDocument {request}.toJson(QJsonDocument::Compact) + '\n');
    if (!socket.waitForBytesWritten(750) || !socket.waitForReadyRead(1500)) {
        return {{false, 5904, QStringLiteral("P4 projection service did not respond")}, 0};
    }
    const QJsonObject response = QJsonDocument::fromJson(socket.readAll().trimmed()).object();
    if (response.contains(QStringLiteral("error"))) {
        const QJsonObject error = response.value(QStringLiteral("error")).toObject();
        return {{false, error.value(QStringLiteral("code")).toInt(5904), error.value(QStringLiteral("message")).toString()}, 0};
    }
    const QJsonObject result = response.value(QStringLiteral("result")).toObject();
    if (result.isEmpty()) return {{false, 5904, QStringLiteral("P4 projection response is invalid")}, 0};
    return {{true, 0, {}}, static_cast<quint64>(result.value(QStringLiteral("frame_id")).toInteger())};
}

} // namespace astra::spatial_ui::service
