#pragma once

#include "input/FrameSource.h"
#include "vision/CalibrationService.h"
#include "vision/SurfaceDetector.h"

#include <QJsonObject>

namespace astra::spatial {

struct SpatialSnapshot {
    SpatialServiceState state {SpatialServiceState::Stopped};
    SceneState sceneState {SceneState::Stopped};
    FrameSourceType inputSource {FrameSourceType::Simulation};
    QVector<SurfaceCandidate> candidates;
    ProjectionTarget target;
    SpatialAnchor anchor;
    SpatialScene scene;
    SceneObject sceneObject;
    ObserverPose observer;
    Vector3 observerCameraOffset;
    QString sceneId;
    int lastErrorCode {0};
    QString warning;
    double processingDurationMs {0.0};
};

class SpatialService final {
public:
    SpatialService(QString projectRoot, QString auditPath);

    SpatialResult<bool> start(FrameSourceType source = FrameSourceType::Simulation, const QString &location = {});
    SpatialResult<bool> stop();
    SpatialResult<SpatialSnapshot> detect();
    SpatialResult<ProjectionTarget> select(bool manualConfirmation);
    SpatialResult<ProjectionTarget> calibrate(const std::array<SpatialPoint, 4> &corners);
    SpatialResult<bool> setObserver(const ObserverPose &observer);
    SpatialResult<bool> reset();
    SpatialSnapshot snapshot() const;
    void setRequestContext(QString traceId, QString requestId);

private:
    void setFailure(SpatialServiceState state, int errorCode, const QString &warning);
    void clearScene();
    void writeCalibrationProfile(const ProjectionTarget &target, double reprojectionErrorPixels) const;
    void writeAudit(const QString &event, const QString &result, int errorCode = 0, const QJsonObject &details = {}) const;
    void markTargetLost();

    QString auditPath_;
    QString projectRoot_;
    FrameSourceController source_;
    SurfaceDetector detector_;
    CalibrationService calibration_;
    SpatialSnapshot snapshot_;
    SpatialFrame latestFrame_;
    QString traceId_;
    QString requestId_;
};

} // namespace astra::spatial
