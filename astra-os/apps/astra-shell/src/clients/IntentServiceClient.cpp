#include "clients/IntentServiceClient.h"
#include "clients/LocalSocketResponse.h"

#include "astra/common/Identifiers.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QLocalSocket>

namespace astra::shell {

IntentServiceClient::IntentServiceClient(QString socketPath, QString capabilityToken, int timeoutMs)
    : socketPath_(std::move(socketPath)), capabilityToken_(std::move(capabilityToken)), timeoutMs_(timeoutMs)
{
}

bool IntentServiceClient::configured() const
{
    return !socketPath_.isEmpty() && !capabilityToken_.isEmpty();
}

IntentServiceResult IntentServiceClient::analyze(const QString &text,
                                                 const QString &privacyLevel,
                                                 const QString &projectionState,
                                                 const QString &currentSpace,
                                                 const QString &currentModelId) const
{
    IntentServiceResult output;
    const QString requestId = QString::fromStdString(astra::common::newUuid());
    const QJsonObject context {{QStringLiteral("projection_state"), projectionState},
                               {QStringLiteral("current_space"), currentSpace},
                               {QStringLiteral("current_privacy_level"), privacyLevel},
                               {QStringLiteral("current_model_id"), currentModelId.isEmpty() ? QJsonValue::Null : QJsonValue(currentModelId)}};
    const QJsonObject params {{QStringLiteral("schema_version"), QStringLiteral("2.0")},
                              {QStringLiteral("request_id"), requestId},
                              {QStringLiteral("session_id"), QString::fromStdString(astra::common::newUuid())},
                              {QStringLiteral("user_id"), QStringLiteral("local-user")},
                              {QStringLiteral("locale"), QStringLiteral("zh-CN")},
                              {QStringLiteral("raw_text"), text},
                              {QStringLiteral("current_context"), context},
                              {QStringLiteral("client"), QJsonObject {{QStringLiteral("name"), QStringLiteral("astra-shell")},
                                                                      {QStringLiteral("version"), QStringLiteral("0.2.0-alpha.1")}}},
                              {QStringLiteral("requested_at"), QString::fromStdString(astra::common::utcTimestamp())}};
    int transportError = 0;
    QString transportMessage;
    const QJsonObject envelope = call(QStringLiteral("intent.parse"), params, transportError, transportMessage);
    if (transportError != 0) {
        output.errorCode = transportError;
        output.errorMessage = transportMessage;
        return output;
    }
    if (envelope.contains(QStringLiteral("error"))) {
        const QJsonObject error = envelope.value(QStringLiteral("error")).toObject();
        output.transportOk = true;
        output.errorCode = error.value(QStringLiteral("code")).toInt();
        output.errorMessage = error.value(QStringLiteral("message")).toString();
        return output;
    }
    const QJsonObject result = envelope.value(QStringLiteral("result")).toObject();
    output.transportOk = true;
    output.requestId = result.value(QStringLiteral("request_id")).toString();
    output.traceId = result.value(QStringLiteral("trace_id")).toString();
    output.intent = result.value(QStringLiteral("intent")).toString(QStringLiteral("unknown"));
    output.confidence = result.value(QStringLiteral("confidence")).toDouble();
    output.executionPolicy = result.value(QStringLiteral("execution_policy")).toString(QStringLiteral("REJECT"));
    output.requiresConfirmation = result.value(QStringLiteral("requires_confirmation")).toBool();
    output.ambiguous = result.value(QStringLiteral("ambiguous")).toBool();
    output.candidates = result.value(QStringLiteral("candidates")).toArray().toVariantList();
    output.slotValues = result.value(QStringLiteral("slots")).toObject().toVariantMap();
    output.normalizedText = result.value(QStringLiteral("normalized_text")).toString();
    const QJsonObject processing = result.value(QStringLiteral("processing")).toObject();
    output.ruleEngineUsed = processing.value(QStringLiteral("rule_engine_used")).toBool();
    output.localModelUsed = processing.value(QStringLiteral("local_model_used")).toBool();
    output.fallbackUsed = processing.value(QStringLiteral("fallback_used")).toBool();
    output.cacheHit = processing.value(QStringLiteral("cache_hit")).toBool();
    output.durationMs = processing.value(QStringLiteral("duration_ms")).toDouble();
    for (const auto &warning : result.value(QStringLiteral("warnings")).toArray()) output.warnings.append(warning.toString());
    const QJsonValue errorValue = result.value(QStringLiteral("error"));
    if (errorValue.isObject()) {
        output.errorCode = errorValue.toObject().value(QStringLiteral("code")).toInt();
        output.errorMessage = errorValue.toObject().value(QStringLiteral("message")).toString();
    }
    const QJsonValue clarificationValue = result.value(QStringLiteral("clarification"));
    if (clarificationValue.isObject()) {
        const QJsonObject clarification = clarificationValue.toObject();
        output.clarificationQuestion = clarification.value(QStringLiteral("question")).toString();
        for (const auto &option : clarification.value(QStringLiteral("options")).toArray()) output.clarificationOptions.append(option.toString());
    }
    const QJsonValue confirmationValue = result.value(QStringLiteral("confirmation"));
    if (confirmationValue.isObject()) {
        output.confirmationId = confirmationValue.toObject().value(QStringLiteral("confirmation_id")).toString();
        output.confirmationMessage = confirmationValue.toObject().value(QStringLiteral("message")).toString();
    }
    return output;
}

ConfirmationOperationResult IntentServiceClient::confirm(const QString &confirmationId) const
{
    return confirmationOperation(QStringLiteral("intent.confirm"), confirmationId);
}

ConfirmationOperationResult IntentServiceClient::reject(const QString &confirmationId) const
{
    return confirmationOperation(QStringLiteral("intent.reject"), confirmationId);
}

QJsonObject IntentServiceClient::call(const QString &method,
                                      const QJsonObject &params,
                                      int &transportError,
                                      QString &transportMessage) const
{
    if (!configured()) {
        transportError = 2003;
        transportMessage = QStringLiteral("Intent service transport is not configured");
        return {};
    }
    QLocalSocket socket;
    socket.connectToServer(socketPath_, QIODevice::ReadWrite);
    if (!socket.waitForConnected(timeoutMs_)) {
        transportError = 2003;
        transportMessage = socket.errorString();
        return {};
    }
    const QString rpcId = QString::fromStdString(astra::common::newUuid());
    const QJsonObject request {{QStringLiteral("jsonrpc"), QStringLiteral("2.0")},
                               {QStringLiteral("id"), rpcId},
                               {QStringLiteral("method"), method},
                               {QStringLiteral("params"), params},
                               {QStringLiteral("security_context"),
                                QJsonObject {{QStringLiteral("capability_token"), capabilityToken_}}}};
    const QByteArray payload = QJsonDocument(request).toJson(QJsonDocument::Compact) + '\n';
    if (socket.write(payload) != payload.size() || !socket.waitForBytesWritten(timeoutMs_)) {
        transportError = 2003;
        transportMessage = socket.errorString();
        return {};
    }
    const auto line = readResponseLine(socket, timeoutMs_);
    if (!line) {
        transportError = 2003;
        transportMessage = socket.errorString();
        return {};
    }
    QJsonParseError parseError;
    const QJsonDocument document = QJsonDocument::fromJson(*line, &parseError);
    if (parseError.error != QJsonParseError::NoError || !document.isObject()) {
        transportError = 2004;
        transportMessage = QStringLiteral("Intent service returned invalid JSON");
        return {};
    }
    return document.object();
}

ConfirmationOperationResult IntentServiceClient::confirmationOperation(const QString &method, const QString &confirmationId) const
{
    ConfirmationOperationResult output;
    int transportError = 0;
    QString transportMessage;
    const QJsonObject envelope = call(method,
                                      {{QStringLiteral("confirmation_id"), confirmationId}},
                                      transportError,
                                      transportMessage);
    if (transportError != 0) {
        output.errorCode = transportError;
        output.errorMessage = transportMessage;
        return output;
    }
    if (envelope.contains(QStringLiteral("error"))) {
        output.transportOk = true;
        output.errorCode = envelope.value(QStringLiteral("error")).toObject().value(QStringLiteral("code")).toInt();
        output.errorMessage = envelope.value(QStringLiteral("error")).toObject().value(QStringLiteral("message")).toString();
        return output;
    }
    output.transportOk = true;
    output.status = envelope.value(QStringLiteral("result")).toObject().value(QStringLiteral("status")).toString();
    return output;
}

} // namespace astra::shell
