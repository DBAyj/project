#include "clients/ProjectionServiceClient.h"

#include "astra/common/Identifiers.h"
#include "astra/common/CapabilityToken.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QLocalSocket>

namespace astra::shell {
namespace {

constexpr auto kDefaultFixtureCapability = "astra-p4-fixture-capability";

} // namespace

ProjectionServiceClient::ProjectionServiceClient(QString socketName, QString capabilityToken, QString framePath)
    : socketName_(std::move(socketName))
    , capabilityToken_(capabilityToken.isEmpty() ? QString::fromLatin1(kDefaultFixtureCapability) : std::move(capabilityToken))
    , framePath_(framePath.isEmpty()
                     ? qEnvironmentVariable("ASTRA_P4_OUTPUT_FRAME_PATH", QDir::current().filePath(QStringLiteral("runtime/cache/p4-output.png")))
                     : std::move(framePath))
{
}

bool ProjectionServiceClient::isConfigured() const { return !socketName_.isEmpty(); }

ProjectionOperationResult ProjectionServiceClient::start()
{
    if (sessionId_.isEmpty()) sessionId_ = QString::fromStdString(astra::common::newUuid());
    auto result = command(QStringLiteral("INITIALIZE"));
    if (!result.ok) return result;
    result = command(QStringLiteral("READY"));
    if (!result.ok) return result;
    result = invoke(QStringLiteral("projection.render"),
                    {{QStringLiteral("request_id"), QString::fromStdString(astra::common::newUuid())},
                     {QStringLiteral("trace_id"), QString::fromStdString(astra::common::newUuid())}, {QStringLiteral("session_id"), sessionId_},
                     {QStringLiteral("output_id"), QStringLiteral("p4-window-projection")}, {QStringLiteral("fixture_id"), QStringLiteral("front-rectangle")},
                     {QStringLiteral("layer_ids"), QJsonArray {QString::fromStdString(astra::common::newUuid())}}, {QStringLiteral("privacy_level"), QStringLiteral("PUBLIC")},
                     {QStringLiteral("p3_integration_status"), QStringLiteral("P3_REAL_SPATIAL_INTEGRATION_NOT_VERIFIED")}});
    if (!result.ok) return result;
    const auto frame = invoke(QStringLiteral("projection.output.frame"), {{QStringLiteral("output_id"), QStringLiteral("p4-window-projection")}});
    if (!frame.ok) return frame;
    state_ = QStringLiteral("ACTIVE");
    result.state = state_;
    return result;
}

ProjectionOperationResult ProjectionServiceClient::pause()
{
    auto result = command(QStringLiteral("PAUSE"));
    if (result.ok) state_ = QStringLiteral("PAUSED");
    return result;
}

ProjectionOperationResult ProjectionServiceClient::resume()
{
    auto result = command(QStringLiteral("RESUME"));
    if (result.ok) {
        state_ = QStringLiteral("ACTIVE");
        result.state = state_;
    }
    return result;
}

ProjectionOperationResult ProjectionServiceClient::stop()
{
    auto result = command(QStringLiteral("STOP"));
    if (result.ok) {
        QFile::remove(framePath_);
        state_ = QStringLiteral("IDLE");
        sessionId_.clear();
        result.sessionId.clear();
        result.state = state_;
    }
    return result;
}

ProjectionOperationResult ProjectionServiceClient::attachToCurrentOutput()
{
    const auto result = invoke(QStringLiteral("projection.output.frame"),
                               {{QStringLiteral("output_id"), QStringLiteral("p4-window-projection")}});
    if (!result.ok) {
        detachOutput();
        return result;
    }
    state_ = QStringLiteral("ACTIVE");
    return {true, 0, {}, sessionId_, state_};
}

void ProjectionServiceClient::detachOutput()
{
    QFile::remove(framePath_);
    state_ = QStringLiteral("IDLE");
    sessionId_.clear();
}

QString ProjectionServiceClient::state() const { return state_; }

QString ProjectionServiceClient::sessionId() const { return sessionId_; }

QString ProjectionServiceClient::framePath() const { return framePath_; }

ProjectionOperationResult ProjectionServiceClient::decodeResponse(const QJsonObject &response)
{
    if (response.contains(QStringLiteral("error"))) {
        const QJsonObject error = response.value(QStringLiteral("error")).toObject();
        return {false, error.value(QStringLiteral("code")).toInt(), error.value(QStringLiteral("message")).toString(), {}, QStringLiteral("ERROR")};
    }
    const QJsonObject result = response.value(QStringLiteral("result")).toObject();
    if (result.isEmpty()) return {false, 9001, QStringLiteral("Invalid JSON-RPC response"), {}, QStringLiteral("ERROR")};
    return {true, 0, {}, result.value(QStringLiteral("session_id")).toString(), result.value(QStringLiteral("state")).toString()};
}

ProjectionOperationResult ProjectionServiceClient::invoke(const QString &method, const QJsonObject &params)
{
    if (!isConfigured()) return {false, 7001, QStringLiteral("Projection service socket is not configured"), {}, QStringLiteral("ERROR")};
    const QString capability = astra::common::projectionCapabilityForMethod(method);
    if (capability.isEmpty()) return {false, 5002, QStringLiteral("Projection method has no capability mapping"), {}, QStringLiteral("ERROR")};
    QLocalSocket socket;
    socket.connectToServer(socketName_);
    if (!socket.waitForConnected(500)) return {false, 7001, QStringLiteral("Projection service is unavailable"), {}, QStringLiteral("ERROR")};
    const QJsonObject request {{QStringLiteral("jsonrpc"), QStringLiteral("2.0")},
                               {QStringLiteral("id"), QString::fromStdString(astra::common::newUuid())},
                               {QStringLiteral("trace_id"), QString::fromStdString(astra::common::newUuid())},
                               {QStringLiteral("method"), method}, {QStringLiteral("params"), params},
                               {QStringLiteral("security_context"), QJsonObject {{QStringLiteral("capability_token"),
                                                                                    astra::common::scopedCapabilityToken(capabilityToken_, capability)}}}};
    socket.write(QJsonDocument(request).toJson(QJsonDocument::Compact) + '\n');
    if (!socket.waitForBytesWritten(500) || !socket.waitForReadyRead(1000)) {
        return {false, 7001, QStringLiteral("Projection service did not respond"), {}, QStringLiteral("ERROR")};
    }
    const QJsonObject response = QJsonDocument::fromJson(socket.readAll().trimmed()).object();
    const auto decoded = decodeResponse(response);
    if (decoded.ok) {
        if (method == QStringLiteral("projection.output.frame")) {
            const QByteArray png = QByteArray::fromBase64(response.value(QStringLiteral("result")).toObject().value(QStringLiteral("frame_png_base64")).toString().toLatin1());
            const QFileInfo fileInfo(framePath_);
            if (png.isEmpty() || !QDir().mkpath(fileInfo.dir().path())) {
                return {false, 4018, QStringLiteral("Projection output frame cache is unavailable"), {}, QStringLiteral("ERROR")};
            }
            QFile file(framePath_);
            if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate) || file.write(png) != png.size()) {
                return {false, 4018, QStringLiteral("Projection output frame cache write failed"), {}, QStringLiteral("ERROR")};
            }
        }
        if (!decoded.sessionId.isEmpty()) sessionId_ = decoded.sessionId;
        if (!decoded.state.isEmpty()) state_ = p1State(decoded.state);
        return {true, 0, {}, sessionId_, state_};
    }
    return decoded;
}

ProjectionOperationResult ProjectionServiceClient::command(const QString &command)
{
    return invoke(QStringLiteral("projection.session.command"), {{QStringLiteral("session_id"), sessionId_}, {QStringLiteral("command"), command}});
}

QString ProjectionServiceClient::p1State(const QString &p4State)
{
    if (p4State == QStringLiteral("RENDERING")) return QStringLiteral("ACTIVE");
    if (p4State == QStringLiteral("PAUSED")) return QStringLiteral("PAUSED");
    if (p4State == QStringLiteral("STOPPED") || p4State == QStringLiteral("CLEARED")) return QStringLiteral("IDLE");
    return p4State;
}

} // namespace astra::shell
