#pragma once

#include "astra/projection/ProjectionRuntime.h"
#include "astra/projection/ProjectionSession.h"
#include "astra/render/ProjectionOutput.h"

#include <QJsonObject>
#include <QString>
#include <QStringList>

#include <memory>

class QLocalServer;

namespace astra::projection::service {

inline constexpr const char *kFixtureCapabilityToken = "astra-p4-fixture-capability";

class ProjectionService final {
public:
    explicit ProjectionService(QString capabilityToken = QString::fromLatin1(kFixtureCapabilityToken));
    ~ProjectionService();

    bool listen(const QString &socketName, QString *errorMessage = nullptr);
    void close();
    QJsonObject handleRequest(const QJsonObject &request);
    QString sessionId() const;
    bool outputHasContent() const;

private:
    QJsonObject success(const QString &requestId, const QJsonObject &result) const;
    QJsonObject failure(const QString &requestId, int code, const QString &message) const;
    QJsonObject handleCommand(const QString &requestId, const QJsonObject &params);
    QJsonObject handleRender(const QString &requestId, const QJsonObject &params);
    QJsonObject handleLayerSubmit(const QString &requestId, const QJsonObject &params);
    bool authorized(const QJsonObject &request) const;

    astra::render::WindowProjectionOutput output_ {QStringLiteral("p4-window-projection")};
    astra::projection::ProjectionSession session_;
    astra::projection::ProjectionRuntime runtime_;
    std::unique_ptr<QLocalServer> server_;
    QString socketName_;
    QString capabilityToken_;
    QString lastSource_ {QStringLiteral("P4_FIXTURE")};
    QStringList lastPublicLabels_;
    QString lastP3IntegrationStatus_ {QString::fromLatin1(kP3FixtureOnlyMarker)};
    QString lastSpatialTargetId_;
    quint64 lastFrameId_ {0};
};

} // namespace astra::projection::service
