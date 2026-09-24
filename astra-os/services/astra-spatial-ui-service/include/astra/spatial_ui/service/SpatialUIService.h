#pragma once

#include "astra/spatial_ui/service/SpatialUIRuntime.h"

#include <QJsonObject>
#include <QSet>
#include <QDateTime>
#include <QTimer>

#include <memory>
#include <functional>

class QLocalServer;
class QHostAddress;
class QTcpServer;
class QTcpSocket;

namespace astra::spatial_ui::service {

inline constexpr const char *kP4ReleaseMarker = "P4_RELEASE_BASELINE_FINAL";

class SpatialUIService final {
public:
    SpatialUIService(QString statePath,
                     QString auditPath,
                     QString clientToken,
                     QString supervisorToken,
                     std::shared_ptr<ProjectionGateway> projectionGateway = {},
                     SpatialUIRuntimeOptions options = {});
    ~SpatialUIService();

    bool listen(const QString &socketPath, QString *errorMessage = nullptr);
    bool listenHttp(const QHostAddress &address, quint16 port, QString *errorMessage = nullptr);
    void close();
    [[nodiscard]] QJsonObject handleRequest(const QJsonObject &request);
    [[nodiscard]] QString socketPath() const;
    [[nodiscard]] quint16 httpPort() const;
    void setShutdownHandler(std::function<void()> handler);

private:
    [[nodiscard]] QJsonObject success(const QString &id, const QJsonObject &result) const;
    [[nodiscard]] QJsonObject failure(const QString &id, int code, const QString &message) const;
    [[nodiscard]] bool authorized(const QJsonObject &request, const QString &method) const;
    void handleHttpRequest(QTcpSocket *socket, const QByteArray &request);
    static void writeHttpResponse(QTcpSocket *socket, int status, const QJsonObject &payload);
    void publishEvent(const QJsonObject &event);
    void startMaintenance();
    void performMaintenance();
    void audit(const QString &event,
               const QString &traceId,
               const QString &requestId,
               const QString &result,
               int errorCode = 0,
               const QString &privacyLevel = QStringLiteral("PRIVATE_SCREEN_ONLY"),
               const QJsonObject &payload = {});

    SpatialUIRuntime runtime_;
    QString auditPath_;
    QString clientToken_;
    QString supervisorToken_;
    QString sessionId_;
    QString socketPath_;
    std::unique_ptr<QLocalServer> server_;
    std::unique_ptr<QTcpServer> httpServer_;
    QSet<QTcpSocket *> eventSubscribers_;
    std::function<void()> shutdownHandler_;
    QTimer maintenanceTimer_;
    QDateTime lastAutosaveAt_;
    int autosaveIntervalMilliseconds_ {10000};
    bool statePersistenceEnabled_ {true};
    bool stopAudited_ {false};
};

} // namespace astra::spatial_ui::service
