#pragma once

#include "astra/render/ProjectionLayer.h"
#include "astra/ui/Types.h"

#include <QJsonObject>
#include <QString>
#include <QVector>

namespace astra::spatial_ui::service {

struct ProjectionGatewayResult : astra::ui::OperationResult {
    quint64 frameId {0};
};

class ProjectionGateway {
public:
    virtual ~ProjectionGateway() = default;
    virtual ProjectionGatewayResult submit(const QVector<astra::render::ProjectionLayer> &layers,
                                           const QString &traceId,
                                           const QString &requestId) = 0;
    virtual astra::ui::OperationResult clear(const QString &reason,
                                             const QString &traceId,
                                             const QString &requestId) = 0;
};

class UnixProjectionGateway final : public ProjectionGateway {
public:
    UnixProjectionGateway(QString socketPath, QString capabilityToken,
                          QString spatialSocketPath = {}, QString spatialCapabilityToken = {});

    ProjectionGatewayResult submit(const QVector<astra::render::ProjectionLayer> &layers,
                                   const QString &traceId,
                                   const QString &requestId) override;
    astra::ui::OperationResult clear(const QString &reason,
                                     const QString &traceId,
                                     const QString &requestId) override;

private:
    ProjectionGatewayResult ensureSession(const QString &traceId, const QString &requestId);
    ProjectionGatewayResult invoke(const QString &method,
                                   const QJsonObject &params,
                                   const QString &traceId,
                                   const QString &requestId) const;
    [[nodiscard]] QJsonObject verifiedSpatialTarget(const QString &traceId, const QString &requestId) const;

    QString socketPath_;
    QString capabilityToken_;
    QString spatialSocketPath_;
    QString spatialCapabilityToken_;
    QString sessionId_;
};

} // namespace astra::spatial_ui::service
