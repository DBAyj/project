#include "models/IntentResultModel.h"

namespace astra::shell {

IntentResultModel::IntentResultModel(QObject *parent) : QObject(parent) { }

QString IntentResultModel::serviceStatus() const { return serviceStatus_; }
QString IntentResultModel::intent() const { return intent_; }
double IntentResultModel::confidence() const { return confidence_; }
QString IntentResultModel::executionPolicy() const { return executionPolicy_; }
bool IntentResultModel::requiresConfirmation() const { return requiresConfirmation_; }
bool IntentResultModel::ambiguous() const { return ambiguous_; }
QVariantList IntentResultModel::candidates() const { return candidates_; }
QVariantMap IntentResultModel::slotValues() const { return slotValues_; }
QString IntentResultModel::ruleEngineStatus() const { return ruleEngineStatus_; }
QString IntentResultModel::localModelStatus() const { return localModelStatus_; }
bool IntentResultModel::fallbackUsed() const { return fallbackUsed_; }
bool IntentResultModel::cacheHit() const { return cacheHit_; }
double IntentResultModel::durationMs() const { return durationMs_; }
QString IntentResultModel::clarificationQuestion() const { return clarificationQuestion_; }
QStringList IntentResultModel::clarificationOptions() const { return clarificationOptions_; }
QString IntentResultModel::confirmationId() const { return confirmationId_; }
QString IntentResultModel::confirmationMessage() const { return confirmationMessage_; }
QString IntentResultModel::errorMessage() const { return errorMessage_; }

void IntentResultModel::apply(const IntentServiceResult &result)
{
    serviceStatus_ = QStringLiteral("ONLINE");
    intent_ = result.intent;
    confidence_ = result.confidence;
    executionPolicy_ = result.executionPolicy;
    requiresConfirmation_ = result.requiresConfirmation;
    ambiguous_ = result.ambiguous;
    candidates_ = result.candidates;
    slotValues_ = result.slotValues;
    ruleEngineStatus_ = result.ruleEngineUsed ? QStringLiteral("READY") : QStringLiteral("DEGRADED");
    localModelStatus_ = result.localModelUsed ? QStringLiteral("READY") : QStringLiteral("DEGRADED");
    fallbackUsed_ = result.fallbackUsed;
    cacheHit_ = result.cacheHit;
    durationMs_ = result.durationMs;
    clarificationQuestion_ = result.clarificationQuestion;
    clarificationOptions_ = result.clarificationOptions;
    confirmationId_ = result.confirmationId;
    confirmationMessage_ = result.confirmationMessage;
    errorMessage_ = result.errorMessage;
    emit changed();
}

void IntentResultModel::setOfflineFallback(const QString &message)
{
    serviceStatus_ = QStringLiteral("OFFLINE_FALLBACK");
    intent_ = QStringLiteral("unknown");
    confidence_ = 0.0;
    executionPolicy_ = QStringLiteral("REJECT");
    requiresConfirmation_ = false;
    ambiguous_ = false;
    candidates_.clear();
    slotValues_.clear();
    ruleEngineStatus_ = QStringLiteral("OFFLINE");
    localModelStatus_ = QStringLiteral("OFFLINE");
    fallbackUsed_ = true;
    cacheHit_ = false;
    durationMs_ = 0.0;
    clarificationQuestion_.clear();
    clarificationOptions_.clear();
    confirmationId_.clear();
    confirmationMessage_.clear();
    errorMessage_ = message;
    emit changed();
}

void IntentResultModel::clearConfirmation()
{
    requiresConfirmation_ = false;
    confirmationId_.clear();
    confirmationMessage_.clear();
    emit changed();
}

} // namespace astra::shell
