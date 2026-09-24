#pragma once

#include <QJsonObject>
#include <QString>

namespace astra::shell {

struct SpatialServiceResult {
    bool transportOk {false};
    QJsonObject result;
    int errorCode {0};
    QString errorMessage;
};

class SpatialServiceClient final {
public:
    SpatialServiceClient(QString socketPath = {}, QString capabilityToken = {}, int timeoutMs = 1000);

    SpatialServiceResult call(const QString &method, const QJsonObject &params = {}) const;

private:
    QString socketPath_;
    QString capabilityToken_;
    int timeoutMs_ {1000};
};

} // namespace astra::shell
