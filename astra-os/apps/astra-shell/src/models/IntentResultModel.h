#pragma once

#include "clients/IntentServiceClient.h"

#include <QObject>

namespace astra::shell {

class IntentResultModel final : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString serviceStatus READ serviceStatus NOTIFY changed)
    Q_PROPERTY(QString intent READ intent NOTIFY changed)
    Q_PROPERTY(double confidence READ confidence NOTIFY changed)
    Q_PROPERTY(QString executionPolicy READ executionPolicy NOTIFY changed)
    Q_PROPERTY(bool requiresConfirmation READ requiresConfirmation NOTIFY changed)
    Q_PROPERTY(bool ambiguous READ ambiguous NOTIFY changed)
    Q_PROPERTY(QVariantList candidates READ candidates NOTIFY changed)
    Q_PROPERTY(QVariantMap slots READ slotValues NOTIFY changed)
    Q_PROPERTY(QString ruleEngineStatus READ ruleEngineStatus NOTIFY changed)
    Q_PROPERTY(QString localModelStatus READ localModelStatus NOTIFY changed)
    Q_PROPERTY(bool fallbackUsed READ fallbackUsed NOTIFY changed)
    Q_PROPERTY(bool cacheHit READ cacheHit NOTIFY changed)
    Q_PROPERTY(double durationMs READ durationMs NOTIFY changed)
    Q_PROPERTY(QString clarificationQuestion READ clarificationQuestion NOTIFY changed)
    Q_PROPERTY(QStringList clarificationOptions READ clarificationOptions NOTIFY changed)
    Q_PROPERTY(QString confirmationId READ confirmationId NOTIFY changed)
    Q_PROPERTY(QString confirmationMessage READ confirmationMessage NOTIFY changed)
    Q_PROPERTY(QString errorMessage READ errorMessage NOTIFY changed)

public:
    explicit IntentResultModel(QObject *parent = nullptr);

    QString serviceStatus() const;
    QString intent() const;
    double confidence() const;
    QString executionPolicy() const;
    bool requiresConfirmation() const;
    bool ambiguous() const;
    QVariantList candidates() const;
    QVariantMap slotValues() const;
    QString ruleEngineStatus() const;
    QString localModelStatus() const;
    bool fallbackUsed() const;
    bool cacheHit() const;
    double durationMs() const;
    QString clarificationQuestion() const;
    QStringList clarificationOptions() const;
    QString confirmationId() const;
    QString confirmationMessage() const;
    QString errorMessage() const;

    void apply(const IntentServiceResult &result);
    void setOfflineFallback(const QString &message);
    void clearConfirmation();

signals:
    void changed();

private:
    QString serviceStatus_ {QStringLiteral("IDLE")};
    QString intent_ {QStringLiteral("unknown")};
    double confidence_ {0.0};
    QString executionPolicy_ {QStringLiteral("REJECT")};
    bool requiresConfirmation_ {false};
    bool ambiguous_ {false};
    QVariantList candidates_;
    QVariantMap slotValues_;
    QString ruleEngineStatus_ {QStringLiteral("IDLE")};
    QString localModelStatus_ {QStringLiteral("IDLE")};
    bool fallbackUsed_ {false};
    bool cacheHit_ {false};
    double durationMs_ {0.0};
    QString clarificationQuestion_;
    QStringList clarificationOptions_;
    QString confirmationId_;
    QString confirmationMessage_;
    QString errorMessage_;
};

} // namespace astra::shell
