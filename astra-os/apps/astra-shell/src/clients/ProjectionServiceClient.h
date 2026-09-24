#pragma once

#include "services/ProjectionSessionService.h"

#include <QJsonObject>
#include <QString>

namespace astra::shell {

class ProjectionServiceClient final {
public:
    explicit ProjectionServiceClient(QString socketName = {}, QString capabilityToken = {}, QString framePath = {});

    bool isConfigured() const;
    ProjectionOperationResult start();
    ProjectionOperationResult pause();
    ProjectionOperationResult resume();
    ProjectionOperationResult stop();
    ProjectionOperationResult attachToCurrentOutput();
    void detachOutput();
    QString state() const;
    QString sessionId() const;
    QString framePath() const;
    static ProjectionOperationResult decodeResponse(const QJsonObject &response);

private:
    ProjectionOperationResult invoke(const QString &method, const QJsonObject &params);
    ProjectionOperationResult command(const QString &command);
    static QString p1State(const QString &p4State);

    QString socketName_;
    QString capabilityToken_;
    QString framePath_;
    QString state_ {QStringLiteral("IDLE")};
    QString sessionId_;
};

} // namespace astra::shell
