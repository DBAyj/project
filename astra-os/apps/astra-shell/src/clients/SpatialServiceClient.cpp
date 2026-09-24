#include "clients/SpatialServiceClient.h"
#include "clients/LocalSocketResponse.h"

#include "astra/common/Identifiers.h"

#include <QJsonDocument>
#include <QLocalSocket>

namespace astra::shell {

SpatialServiceClient::SpatialServiceClient(QString socketPath, QString capabilityToken, int timeoutMs)
    : socketPath_(std::move(socketPath)), capabilityToken_(std::move(capabilityToken)), timeoutMs_(timeoutMs)
{
}

SpatialServiceResult SpatialServiceClient::call(const QString &method, const QJsonObject &params) const
{
    if (socketPath_.isEmpty() || capabilityToken_.isEmpty()) {
        return {false, {}, 3001, QStringLiteral("Spatial service is not configured")};
    }
    QLocalSocket socket;
    socket.connectToServer(socketPath_);
    if (!socket.waitForConnected(timeoutMs_)) return {false, {}, 3001, QStringLiteral("Spatial service is unavailable")};
    const QJsonObject request {{QStringLiteral("jsonrpc"), QStringLiteral("2.0")},
                               {QStringLiteral("id"), QString::fromStdString(astra::common::newUuid())},
                               {QStringLiteral("trace_id"), QString::fromStdString(astra::common::newUuid())},
                               {QStringLiteral("method"), method},
                               {QStringLiteral("params"), params},
                               {QStringLiteral("security_context"), QJsonObject {{QStringLiteral("capability_token"), capabilityToken_}}}};
    const QByteArray payload = QJsonDocument(request).toJson(QJsonDocument::Compact) + '\n';
    const auto line = socket.write(payload) == payload.size() && socket.waitForBytesWritten(timeoutMs_)
        ? readResponseLine(socket, timeoutMs_) : std::nullopt;
    if (!line) {
        return {false, {}, 3001, QStringLiteral("Spatial service did not respond")};
    }
    QJsonParseError parseError;
    const QJsonObject response = QJsonDocument::fromJson(*line, &parseError).object();
    if (parseError.error != QJsonParseError::NoError || response.value(QStringLiteral("jsonrpc")) != QStringLiteral("2.0")) {
        return {false, {}, 3504, QStringLiteral("Spatial service returned an invalid response")};
    }
    if (response.contains(QStringLiteral("error"))) {
        const QJsonObject error = response.value(QStringLiteral("error")).toObject();
        return {false, {}, error.value(QStringLiteral("code")).toInt(3504), error.value(QStringLiteral("message")).toString()};
    }
    return {true, response.value(QStringLiteral("result")).toObject(), 0, {}};
}

} // namespace astra::shell
