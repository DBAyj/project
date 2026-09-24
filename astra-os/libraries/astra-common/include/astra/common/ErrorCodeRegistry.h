#pragma once

#include <QJsonDocument>
#include <QString>
#include <QVector>

namespace astra::common {

enum class ErrorSeverity { Warning, Error, Critical };

struct ErrorDescriptor {
    int code;
    QString symbol;
    QString messageZh;
    QString messageEn;
    QString module;
    ErrorSeverity severity;
    bool retryable;
    bool auditRequired;
};

class ErrorCodeRegistry final {
public:
    static QVector<ErrorDescriptor> all();
    static ErrorDescriptor lookup(int code);
    static QJsonDocument exportJson();
};

} // namespace astra::common
