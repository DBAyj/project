#pragma once

#include <QJsonObject>
#include <QString>

namespace astra::shell {

struct SpatialUIClientResult {
    bool ok {false};
    int errorCode {0};
    QString message;
    QJsonObject result;
};

class SpatialUIServiceClient final {
public:
    explicit SpatialUIServiceClient(QString socketPath = {}, QString capabilityToken = {});
    [[nodiscard]] bool isConfigured() const;
    SpatialUIClientResult status();
    SpatialUIClientResult components();
    SpatialUIClientResult createTaskCard(const QString &title, const QString &summary, const QString &privacyLevel);
    SpatialUIClientResult activateComponent(const QString &componentId, const QString &action);
    SpatialUIClientResult routeComponentInput(const QString &componentId,
                                              const QString &eventType,
                                              const QString &sourceType,
                                              const QString &coordinateSystem,
                                              double x,
                                              double y);
    SpatialUIClientResult notifyTargetLost();
    static SpatialUIClientResult decodeResponse(const QJsonObject &response);

private:
    SpatialUIClientResult invoke(const QString &method, const QJsonObject &params);
    QString socketPath_;
    QString capabilityToken_;
};

} // namespace astra::shell
