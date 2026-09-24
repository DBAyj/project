#pragma once

#include "application/SpatialService.h"

#include <QLocalServer>
#include <QLocalSocket>
#include <QObject>
#include <QSet>

#include <unordered_map>

namespace astra::spatial {

class SpatialJsonRpcServer final : public QObject {
public:
    SpatialJsonRpcServer(SpatialService *service,
                         QString capabilityToken,
                         QStringList grantedCapabilities = {},
                         QObject *parent = nullptr);

    bool listen(const QString &socketPath);
    void close();
    void publishStateChangedIfChanged();

private:
    void acceptConnections();
    void readConnection(QLocalSocket *socket);
    QJsonObject dispatch(const QJsonObject &request, QLocalSocket *socket);
    static QJsonObject stateChangedPayload(const SpatialSnapshot &snapshot);
    static bool validEnvelope(const QJsonObject &request);
    static bool validParams(const QString &method, const QJsonObject &params);
    static QString requiredCapability(const QString &method);
    static QJsonObject snapshotJson(const SpatialSnapshot &snapshot);
    static QJsonObject targetJson(const ProjectionTarget &target);
    static QJsonObject errorResponse(const QJsonValue &id, int code, const QString &message);
    static QJsonObject successResponse(const QJsonValue &id, const QJsonObject &result);

    SpatialService *service_ {nullptr};
    QString capabilityToken_;
    QSet<QString> grantedCapabilities_;
    QSet<QLocalSocket *> stateSubscribers_;
    QJsonObject lastPublishedState_;
    QString socketPath_;
    QLocalServer server_;
    std::unordered_map<QLocalSocket *, QByteArray> buffers_;
};

} // namespace astra::spatial
