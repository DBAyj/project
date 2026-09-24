#include "clients/SpatialUIServiceClient.h"

#include "astra/common/Identifiers.h"
#include "astra/common/CapabilityToken.h"

#include <QJsonDocument>
#include <QDateTime>
#include <QJsonArray>
#include <QLocalSocket>

namespace astra::shell {

SpatialUIServiceClient::SpatialUIServiceClient(QString socketPath, QString capabilityToken)
    : socketPath_(std::move(socketPath))
    , capabilityToken_(std::move(capabilityToken))
{
}

bool SpatialUIServiceClient::isConfigured() const { return !socketPath_.isEmpty() && !capabilityToken_.isEmpty(); }
SpatialUIClientResult SpatialUIServiceClient::status() { return invoke(QStringLiteral("spatial_ui.status"), {}); }
SpatialUIClientResult SpatialUIServiceClient::components() { return invoke(QStringLiteral("spatial_ui.components"), {}); }

SpatialUIClientResult SpatialUIServiceClient::createTaskCard(const QString &title, const QString &summary, const QString &privacyLevel)
{
    const QString id = QString::fromStdString(astra::common::newUuid());
    return invoke(QStringLiteral("spatial_ui.task.create"),
                  {{QStringLiteral("schema_version"), QStringLiteral("1.0")}, {QStringLiteral("task_id"), id},
                   {QStringLiteral("title"), title}, {QStringLiteral("summary"), summary.isEmpty() ? title : summary},
                   {QStringLiteral("intent_type"), QStringLiteral("fixture_intent")}, {QStringLiteral("confidence"), 1.0},
                   {QStringLiteral("execution_strategy"), QStringLiteral("fixture_adapter")}, {QStringLiteral("privacy_level"), privacyLevel},
                   {QStringLiteral("accessibility_label"), summary.isEmpty() ? title : title + QStringLiteral(": ") + summary},
                   {QStringLiteral("bounds"), QJsonObject {{QStringLiteral("x"), 32.0}, {QStringLiteral("y"), 96.0},
                                                            {QStringLiteral("width"), 360.0}, {QStringLiteral("height"), 180.0}}},
                   {QStringLiteral("display_target"), QStringLiteral("BOTH")}});
}

SpatialUIClientResult SpatialUIServiceClient::activateComponent(const QString &componentId, const QString &action)
{
    return invoke(QStringLiteral("spatial_ui.input"),
                  {{QStringLiteral("schema_version"), QStringLiteral("1.0")},
                   {QStringLiteral("event_id"), QString::fromStdString(astra::common::newUuid())},
                   {QStringLiteral("event_type"), QStringLiteral("AI_ACTION")},
                   {QStringLiteral("source_type"), QStringLiteral("SYSTEM")},
                   {QStringLiteral("source_id"), QStringLiteral("astra-shell")},
                   {QStringLiteral("target_component_id"), componentId}, {QStringLiteral("position"), QJsonValue {QJsonValue::Null}},
                   {QStringLiteral("action"), action.left(128)},
                   {QStringLiteral("modifiers"), QJsonArray {}},
                   {QStringLiteral("timestamp"), QDateTime::currentDateTimeUtc().toString(Qt::ISODateWithMs)}});
}

SpatialUIClientResult SpatialUIServiceClient::routeComponentInput(const QString &componentId,
                                                                  const QString &eventType,
                                                                  const QString &sourceType,
                                                                  const QString &coordinateSystem,
                                                                  double x,
                                                                  double y)
{
    const bool positional = eventType != QStringLiteral("KEY_PRESS")
        && eventType != QStringLiteral("KEY_RELEASE") && eventType != QStringLiteral("SYSTEM_FOCUS");
    const QJsonValue position = positional
        ? QJsonValue {QJsonObject {{QStringLiteral("coordinate_system"), coordinateSystem},
                                  {QStringLiteral("x"), x}, {QStringLiteral("y"), y}}}
        : QJsonValue {QJsonValue::Null};
    return invoke(QStringLiteral("spatial_ui.input"),
                  {{QStringLiteral("schema_version"), QStringLiteral("1.0")},
                   {QStringLiteral("event_id"), QString::fromStdString(astra::common::newUuid())},
                   {QStringLiteral("event_type"), eventType}, {QStringLiteral("source_type"), sourceType},
                   {QStringLiteral("source_id"), QStringLiteral("astra-shell")},
                   {QStringLiteral("target_component_id"), componentId}, {QStringLiteral("position"), position},
                   {QStringLiteral("modifiers"), QJsonArray {}},
                   {QStringLiteral("timestamp"), QDateTime::currentDateTimeUtc().toString(Qt::ISODateWithMs)}});
}

SpatialUIClientResult SpatialUIServiceClient::notifyTargetLost() { return invoke(QStringLiteral("spatial_ui.target.lost"), {}); }

SpatialUIClientResult SpatialUIServiceClient::decodeResponse(const QJsonObject &response)
{
    if (response.contains(QStringLiteral("error"))) {
        const auto error = response.value(QStringLiteral("error")).toObject();
        return {false, error.value(QStringLiteral("code")).toInt(), error.value(QStringLiteral("message")).toString(), {}};
    }
    if (!response.value(QStringLiteral("result")).isObject()) return {false, 5905, QStringLiteral("Invalid Spatial UI response"), {}};
    return {true, 0, {}, response.value(QStringLiteral("result")).toObject()};
}

SpatialUIClientResult SpatialUIServiceClient::invoke(const QString &method, const QJsonObject &params)
{
    if (!isConfigured()) return {false, 5101, QStringLiteral("Spatial UI service socket is not configured"), {}};
    QLocalSocket socket;
    socket.connectToServer(socketPath_);
    if (!socket.waitForConnected(500)) return {false, 5101, QStringLiteral("Spatial UI service is unavailable"), {}};
    const QJsonObject request {{QStringLiteral("jsonrpc"), QStringLiteral("2.0")},
                               {QStringLiteral("id"), QString::fromStdString(astra::common::newUuid())},
                               {QStringLiteral("trace_id"), QString::fromStdString(astra::common::newUuid())},
                               {QStringLiteral("method"), method}, {QStringLiteral("params"), params},
                               {QStringLiteral("security_context"), QJsonObject {{QStringLiteral("capability_token"),
                                                                                    astra::common::scopedCapabilityToken(capabilityToken_, astra::common::spatialUICapabilityForMethod(method))}}}};
    socket.write(QJsonDocument {request}.toJson(QJsonDocument::Compact) + '\n');
    if (!socket.waitForBytesWritten(500) || !socket.waitForReadyRead(1000)) return {false, 5101, QStringLiteral("Spatial UI service did not respond"), {}};
    return decodeResponse(QJsonDocument::fromJson(socket.readAll().trimmed()).object());
}

} // namespace astra::shell
