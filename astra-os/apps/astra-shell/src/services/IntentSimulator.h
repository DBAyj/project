#pragma once

#include <QString>
#include <QVariantMap>

namespace astra::shell {
struct SimulatedIntent {
    QString traceId;
    QString requestId;
    QString rawText;
    QString intent;
    double confidence {0.0};
    QString targetSpace {"desk"};
    QString privacyLevel {"PUBLIC"};
    QVariantMap parameters;
    QString timestamp;
};
class IntentSimulator {
public:
    SimulatedIntent parse(const QString &text, const QString &privacyLevel = QStringLiteral("PUBLIC")) const;
};
} // namespace astra::shell
