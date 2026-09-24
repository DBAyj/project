#pragma once

#include <QJsonObject>
#include <QString>
#include <QStringList>
#include <QVariantList>
#include <QVariantMap>

namespace astra::shell {

struct IntentServiceResult {
    bool transportOk {false};
    QString requestId;
    QString traceId;
    QString intent {QStringLiteral("unknown")};
    double confidence {0.0};
    QString executionPolicy {QStringLiteral("REJECT")};
    bool requiresConfirmation {false};
    bool ambiguous {false};
    QVariantList candidates;
    QVariantMap slotValues;
    QString normalizedText;
    bool ruleEngineUsed {false};
    bool localModelUsed {false};
    bool fallbackUsed {false};
    bool cacheHit {false};
    double durationMs {0.0};
    QStringList warnings;
    int errorCode {0};
    QString errorMessage;
    QString clarificationQuestion;
    QStringList clarificationOptions;
    QString confirmationId;
    QString confirmationMessage;
};

struct ConfirmationOperationResult {
    bool transportOk {false};
    QString status;
    int errorCode {0};
    QString errorMessage;
};

class IntentServiceClient final {
public:
    IntentServiceClient(QString socketPath = {}, QString capabilityToken = {}, int timeoutMs = 1000);

    bool configured() const;
    IntentServiceResult analyze(const QString &text,
                                const QString &privacyLevel,
                                const QString &projectionState,
                                const QString &currentSpace,
                                const QString &currentModelId) const;
    ConfirmationOperationResult confirm(const QString &confirmationId) const;
    ConfirmationOperationResult reject(const QString &confirmationId) const;

private:
    QJsonObject call(const QString &method, const QJsonObject &params, int &transportError, QString &transportMessage) const;
    ConfirmationOperationResult confirmationOperation(const QString &method, const QString &confirmationId) const;

    QString socketPath_;
    QString capabilityToken_;
    int timeoutMs_ {1000};
};

} // namespace astra::shell
