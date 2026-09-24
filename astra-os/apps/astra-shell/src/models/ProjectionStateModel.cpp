#include "models/ProjectionStateModel.h"

#include <algorithm>

namespace astra::shell {

ProjectionStateModel::ProjectionStateModel(QObject *parent) : QObject(parent) {}

QString ProjectionStateModel::state() const { return state_; }
QString ProjectionStateModel::sessionId() const { return sessionId_; }
QString ProjectionStateModel::privacyLevel() const { return privacyLevel_; }
QString ProjectionStateModel::targetSpace() const { return targetSpace_; }
bool ProjectionStateModel::modelVisible() const { return state_ == QStringLiteral("ACTIVE") || state_ == QStringLiteral("PAUSED"); }
bool ProjectionStateModel::rotationPaused() const { return rotationPaused_; }
double ProjectionStateModel::sceneRotation() const { return sceneRotation_; }
double ProjectionStateModel::zoom() const { return zoom_; }
double ProjectionStateModel::cameraOffsetX() const { return cameraOffsetX_; }
double ProjectionStateModel::cameraOffsetY() const { return cameraOffsetY_; }
double ProjectionStateModel::cameraOffsetZ() const { return cameraOffsetZ_; }

void ProjectionStateModel::applySession(const QString &state, const QString &sessionId, const QString &privacyLevel, const QString &targetSpace)
{
    state_ = state;
    sessionId_ = sessionId;
    privacyLevel_ = privacyLevel;
    targetSpace_ = targetSpace;
    if (state_ != QStringLiteral("PAUSED")) rotationPaused_ = false;
    emit changed();
}

void ProjectionStateModel::setRotationPaused(bool paused)
{
    if (rotationPaused_ == paused) return;
    rotationPaused_ = paused;
    emit changed();
}

void ProjectionStateModel::setView(double sceneRotation, double zoom)
{
    sceneRotation_ = sceneRotation;
    zoom_ = std::clamp(zoom, 0.5, 3.0);
    emit changed();
}

void ProjectionStateModel::setSpatialCameraOffset(double x, double y, double z)
{
    cameraOffsetX_ = std::clamp(x, -90.0, 90.0);
    cameraOffsetY_ = std::clamp(y, -90.0, 90.0);
    cameraOffsetZ_ = std::clamp(z, -90.0, 90.0);
    emit changed();
}

void ProjectionStateModel::clear()
{
    state_ = QStringLiteral("IDLE");
    sessionId_.clear();
    privacyLevel_.clear();
    targetSpace_.clear();
    rotationPaused_ = false;
    sceneRotation_ = 0.0;
    zoom_ = 1.0;
    cameraOffsetX_ = 0.0;
    cameraOffsetY_ = 0.0;
    cameraOffsetZ_ = 0.0;
    emit changed();
}

} // namespace astra::shell
