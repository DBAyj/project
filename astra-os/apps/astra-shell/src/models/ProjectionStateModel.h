#pragma once

#include <QObject>
#include <QString>

namespace astra::shell {

class ProjectionStateModel final : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString state READ state NOTIFY changed)
    Q_PROPERTY(QString sessionId READ sessionId NOTIFY changed)
    Q_PROPERTY(QString privacyLevel READ privacyLevel NOTIFY changed)
    Q_PROPERTY(QString targetSpace READ targetSpace NOTIFY changed)
    Q_PROPERTY(bool modelVisible READ modelVisible NOTIFY changed)
    Q_PROPERTY(bool rotationPaused READ rotationPaused NOTIFY changed)
    Q_PROPERTY(double sceneRotation READ sceneRotation NOTIFY changed)
    Q_PROPERTY(double zoom READ zoom NOTIFY changed)
    Q_PROPERTY(double cameraOffsetX READ cameraOffsetX NOTIFY changed)
    Q_PROPERTY(double cameraOffsetY READ cameraOffsetY NOTIFY changed)
    Q_PROPERTY(double cameraOffsetZ READ cameraOffsetZ NOTIFY changed)

public:
    explicit ProjectionStateModel(QObject *parent = nullptr);

    QString state() const;
    QString sessionId() const;
    QString privacyLevel() const;
    QString targetSpace() const;
    bool modelVisible() const;
    bool rotationPaused() const;
    double sceneRotation() const;
    double zoom() const;
    double cameraOffsetX() const;
    double cameraOffsetY() const;
    double cameraOffsetZ() const;

    void applySession(const QString &state, const QString &sessionId, const QString &privacyLevel, const QString &targetSpace);
    void setRotationPaused(bool paused);
    void setView(double sceneRotation, double zoom);
    void setSpatialCameraOffset(double x, double y, double z);
    void clear();

signals:
    void changed();

private:
    QString state_ {QStringLiteral("IDLE")};
    QString sessionId_;
    QString privacyLevel_;
    QString targetSpace_;
    bool rotationPaused_ {false};
    double sceneRotation_ {0.0};
    double zoom_ {1.0};
    double cameraOffsetX_ {0.0};
    double cameraOffsetY_ {0.0};
    double cameraOffsetZ_ {0.0};
};

} // namespace astra::shell
