#include "application/SpatialService.h"

#include "astra/common/Identifiers.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QElapsedTimer>

namespace astra::spatial {
namespace {

std::array<double, 16> identityTransform()
{
    return {1.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 0.0, 1.0};
}

} // namespace

SpatialService::SpatialService(QString projectRoot, QString auditPath)
    : auditPath_(std::move(auditPath)), projectRoot_(projectRoot), source_(std::move(projectRoot))
{
    snapshot_.observer = {};
    snapshot_.observer.trackingState = "SIMULATED";
}

SpatialResult<bool> SpatialService::start(FrameSourceType source, const QString &location)
{
    snapshot_.state = SpatialServiceState::Starting;
    const auto started = source_.start(source, location);
    if (!started.ok && source == FrameSourceType::Camera) {
        const auto fallback = source_.start(FrameSourceType::Simulation);
        if (fallback.ok) {
            if (snapshot_.target.selected) markTargetLost();
            else clearScene();
            snapshot_.inputSource = FrameSourceType::Simulation;
            snapshot_.state = SpatialServiceState::Ready;
            snapshot_.warning = QStringLiteral("Camera unavailable; using simulation fallback");
            snapshot_.lastErrorCode = started.errorCode;
            writeAudit(QStringLiteral("spatial_fallback_enabled"), QStringLiteral("success"), started.errorCode,
                       {{QStringLiteral("fallback_source"), QStringLiteral("SIMULATION")}});
            return {true, true, 0, {}};
        }
    }
    if (!started.ok) {
        clearScene();
        setFailure(SpatialServiceState::Error, started.errorCode, started.message);
        writeAudit(QStringLiteral("spatial_service_started"), QStringLiteral("error"), started.errorCode);
        return started;
    }
    if (snapshot_.target.selected) markTargetLost();
    else clearScene();
    snapshot_.inputSource = source;
    snapshot_.state = SpatialServiceState::Ready;
    snapshot_.sceneState = SceneState::Empty;
    snapshot_.warning.clear();
    snapshot_.lastErrorCode = 0;
    writeAudit(QStringLiteral("spatial_service_started"), QStringLiteral("success"));
    writeAudit(QStringLiteral("input_source_changed"), QStringLiteral("success"), 0,
               {{QStringLiteral("source"), QString::fromStdString(toString(source))}});
    return {true, true, 0, {}};
}

SpatialResult<bool> SpatialService::stop()
{
    snapshot_.state = SpatialServiceState::Stopping;
    const auto stopped = source_.stop();
    if (!stopped.ok) {
        setFailure(SpatialServiceState::Error, stopped.errorCode, stopped.message);
        return stopped;
    }
    clearScene();
    snapshot_.state = SpatialServiceState::Stopped;
    snapshot_.sceneState = SceneState::Stopped;
    snapshot_.candidates.clear();
    snapshot_.warning.clear();
    snapshot_.lastErrorCode = 0;
    writeAudit(QStringLiteral("spatial_service_stopped"), QStringLiteral("success"));
    return {true, true, 0, {}};
}

SpatialResult<SpatialSnapshot> SpatialService::detect()
{
    if (!source_.running()) return {false, {}, 3001, QStringLiteral("Spatial service is not started")};
    const bool targetAlreadyLost = snapshot_.state == SpatialServiceState::Lost && snapshot_.target.state == ProjectionTargetState::Lost;
    snapshot_.state = SpatialServiceState::Detecting;
    QElapsedTimer elapsed;
    elapsed.start();
    const auto frame = source_.nextFrame();
    if (!frame.ok) {
        if (source_.activeSource() == FrameSourceType::Camera) {
            source_.stop();
            const auto fallback = source_.start(FrameSourceType::Simulation);
            if (fallback.ok) {
                if (snapshot_.target.selected) markTargetLost();
                else clearScene();
                snapshot_.inputSource = FrameSourceType::Simulation;
                snapshot_.state = SpatialServiceState::Ready;
                snapshot_.lastErrorCode = frame.errorCode;
                snapshot_.warning = QStringLiteral("Camera frame unavailable; using simulation fallback");
                writeAudit(QStringLiteral("spatial_fallback_enabled"), QStringLiteral("success"), frame.errorCode,
                           {{QStringLiteral("fallback_source"), QStringLiteral("SIMULATION")}});
                const auto recovered = detect();
                snapshot_.lastErrorCode = frame.errorCode;
                snapshot_.warning = QStringLiteral("Camera frame unavailable; using simulation fallback");
                return {recovered.ok, snapshot_, recovered.errorCode, recovered.message};
            }
        }
        setFailure(SpatialServiceState::Degraded, frame.errorCode, frame.message);
        writeAudit(QStringLiteral("frame_processing_failed"), QStringLiteral("error"), frame.errorCode);
        return {false, snapshot_, frame.errorCode, frame.message};
    }
    latestFrame_ = frame.value.frame;
    const auto detected = detector_.detect(frame.value);
    snapshot_.processingDurationMs = static_cast<double>(elapsed.elapsed());
    if (!detected.ok) {
        setFailure(SpatialServiceState::Degraded, detected.errorCode, detected.message);
        writeAudit(QStringLiteral("frame_processing_failed"), QStringLiteral("error"), detected.errorCode);
        return {false, snapshot_, detected.errorCode, detected.message};
    }
    snapshot_.candidates = detected.value;
    if (snapshot_.candidates.isEmpty()) {
        if (snapshot_.target.selected) {
            if (targetAlreadyLost) snapshot_.state = SpatialServiceState::Lost;
            else markTargetLost();
        } else {
            setFailure(SpatialServiceState::Degraded, 3101, QStringLiteral("No valid surface was detected"));
            writeAudit(QStringLiteral("surface_rejected"), QStringLiteral("rejected"), 3101);
        }
        return {true, snapshot_, 0, {}};
    }
    snapshot_.lastErrorCode = 0;
    snapshot_.warning.clear();
    snapshot_.state = SpatialServiceState::Detecting;
    writeAudit(QStringLiteral("surface_candidate_detected"), QStringLiteral("success"), 0,
               {{QStringLiteral("candidate_count"), snapshot_.candidates.size()}});
    return {true, snapshot_, 0, {}};
}

SpatialResult<ProjectionTarget> SpatialService::select(bool manualConfirmation)
{
    const auto selected = detector_.select(snapshot_.candidates, latestFrame_.resolution, manualConfirmation);
    if (!selected.ok) {
        setFailure(SpatialServiceState::Degraded, selected.errorCode, selected.message);
        writeAudit(QStringLiteral("surface_rejected"), QStringLiteral("rejected"), selected.errorCode);
        return selected;
    }
    snapshot_.target = selected.value;
    snapshot_.sceneState = SceneState::Initializing;
    snapshot_.lastErrorCode = 0;
    snapshot_.warning.clear();
    writeAudit(QStringLiteral("surface_selected"), QStringLiteral("success"), 0,
               {{QStringLiteral("target_id"), QString::fromStdString(snapshot_.target.targetId)}});
    return selected;
}

SpatialResult<ProjectionTarget> SpatialService::calibrate(const std::array<SpatialPoint, 4> &corners)
{
    if (!snapshot_.target.selected) {
        return {false, {}, 3304, QStringLiteral("Projection target does not exist")};
    }
    const auto calibrated = calibration_.calibrate(corners, snapshot_.target.sourceResolution);
    if (!calibrated.ok) {
        setFailure(SpatialServiceState::Degraded, calibrated.errorCode, calibrated.message);
        writeAudit(QStringLiteral("calibration_failed"), QStringLiteral("error"), calibrated.errorCode);
        return {false, {}, calibrated.errorCode, calibrated.message};
    }
    ProjectionTarget target = calibrated.value.target;
    target.targetId = snapshot_.target.targetId;
    target.surfaceId = snapshot_.target.surfaceId;
    target.surfaceType = snapshot_.target.surfaceType;
    target.quality = snapshot_.target.quality;
    target.privacyLevel = snapshot_.target.privacyLevel;
    snapshot_.target = target;
    snapshot_.sceneId = QString::fromStdString(astra::common::newUuid());
    snapshot_.anchor.anchorId = astra::common::newUuid();
    snapshot_.anchor.anchorType = "PROJECTION_SURFACE";
    snapshot_.anchor.coordinateSystem = CoordinateSystem::SurfaceLocal;
    snapshot_.anchor.transform = identityTransform();
    snapshot_.anchor.quality = target.quality;
    snapshot_.anchor.persistent = false;
    snapshot_.anchor.createdAt = astra::common::utcTimestamp();
    snapshot_.anchor.lastUpdatedAt = snapshot_.anchor.createdAt;
    snapshot_.scene = {snapshot_.sceneId.toStdString(), SceneState::Tracking, CoordinateSystem::WorldSimulated,
                       {snapshot_.anchor.anchorId}, {}, snapshot_.anchor.createdAt, snapshot_.anchor.lastUpdatedAt};
    snapshot_.sceneObject = {astra::common::newUuid(), snapshot_.scene.sceneId, "DEBUG", "spatial-target",
                             snapshot_.anchor.anchorId, CoordinateSystem::SurfaceLocal, {}, {}, {1.0, 1.0, 1.0}, true,
                             target.privacyLevel, false, SceneObjectState::Active};
    snapshot_.scene.objectIds = {snapshot_.sceneObject.objectId};
    snapshot_.state = SpatialServiceState::Tracking;
    snapshot_.sceneState = SceneState::Tracking;
    snapshot_.lastErrorCode = 0;
    snapshot_.warning.clear();
    writeAudit(QStringLiteral("calibration_confirmed"), QStringLiteral("success"), 0,
               {{QStringLiteral("target_id"), QString::fromStdString(target.targetId)},
                {QStringLiteral("reprojection_error_pixels"), calibrated.value.reprojectionErrorPixels}});
    writeAudit(QStringLiteral("projection_target_created"), QStringLiteral("success"), 0,
               {{QStringLiteral("anchor_id"), QString::fromStdString(snapshot_.anchor.anchorId)}});
    writeAudit(QStringLiteral("scene_created"), QStringLiteral("success"), 0,
               {{QStringLiteral("scene_id"), snapshot_.sceneId},
                {QStringLiteral("object_id"), QString::fromStdString(snapshot_.sceneObject.objectId)}});
    writeCalibrationProfile(target, calibrated.value.reprojectionErrorPixels);
    return {true, target, 0, {}};
}

SpatialResult<bool> SpatialService::setObserver(const ObserverPose &observer)
{
    const auto finite = [](const Vector3 &value) {
        return std::isfinite(value.x) && std::isfinite(value.y) && std::isfinite(value.z);
    };
    if (observer.coordinateSystem != CoordinateSystem::WorldSimulated || observer.trackingState != "SIMULATED"
        || observer.confidence < 0.0 || observer.confidence > 1.0 || !finite(observer.position) || !finite(observer.orientation)
        || std::abs(observer.position.x) > 3.0 || std::abs(observer.position.y) > 3.0 || observer.position.z < 0.1
        || observer.position.z > 5.0 || std::abs(observer.orientation.x) > 180.0 || std::abs(observer.orientation.y) > 180.0
        || std::abs(observer.orientation.z) > 180.0) {
        return {false, false, 3402, QStringLiteral("Observer data is invalid")};
    }
    snapshot_.observer = observer;
    snapshot_.observerCameraOffset = {(observer.position.x) * 30.0, (observer.position.y - 0.2) * 30.0,
                                      (observer.position.z - 1.5) * 30.0};
    writeAudit(QStringLiteral("observer_position_changed"), QStringLiteral("success"));
    return {true, true, 0, {}};
}

SpatialResult<bool> SpatialService::reset()
{
    clearScene();
    snapshot_.sceneState = source_.running() ? SceneState::Empty : SceneState::Stopped;
    snapshot_.state = source_.running() ? SpatialServiceState::Ready : SpatialServiceState::Stopped;
    snapshot_.lastErrorCode = 0;
    snapshot_.warning.clear();
    writeAudit(QStringLiteral("scene_state_changed"), QStringLiteral("success"), 0,
               {{QStringLiteral("state"), QString::fromStdString(toString(snapshot_.state))}});
    return {true, true, 0, {}};
}

SpatialSnapshot SpatialService::snapshot() const { return snapshot_; }

void SpatialService::setRequestContext(QString traceId, QString requestId)
{
    traceId_ = std::move(traceId);
    requestId_ = std::move(requestId);
}

void SpatialService::setFailure(SpatialServiceState state, int errorCode, const QString &warning)
{
    snapshot_.state = state;
    snapshot_.lastErrorCode = errorCode;
    snapshot_.warning = warning;
    if (state == SpatialServiceState::Lost) snapshot_.sceneState = SceneState::Lost;
    if (state == SpatialServiceState::Degraded) snapshot_.sceneState = SceneState::Degraded;
    if (state == SpatialServiceState::Error) snapshot_.sceneState = SceneState::Error;
    snapshot_.scene.state = snapshot_.sceneState;
    if (!snapshot_.sceneObject.objectId.empty() && state != SpatialServiceState::Tracking) snapshot_.sceneObject.lifecycleState = SceneObjectState::Suspended;
}

void SpatialService::clearScene()
{
    snapshot_.candidates.clear();
    snapshot_.target = {};
    snapshot_.anchor = {};
    snapshot_.scene = {};
    snapshot_.sceneObject = {};
    snapshot_.sceneId.clear();
}

void SpatialService::writeCalibrationProfile(const ProjectionTarget &target, double reprojectionErrorPixels) const
{
    const QString directory = QDir(projectRoot_).filePath(QStringLiteral("runtime/spatial/calibration"));
    if (!QDir().mkpath(directory)) return;
    QJsonArray corners;
    for (const auto &corner : target.corners) {
        corners.append(QJsonObject {{QStringLiteral("x"), corner.x}, {QStringLiteral("y"), corner.y},
                                    {QStringLiteral("coordinate_system"), QStringLiteral("IMAGE_PIXEL")}});
    }
    QJsonArray homography;
    QJsonArray inverseHomography;
    for (double value : target.homography) homography.append(value);
    for (double value : target.inverseHomography) inverseHomography.append(value);
    const QString profileId = QString::fromStdString(astra::common::newUuid());
    const QJsonObject profile {{QStringLiteral("schema_version"), QStringLiteral("1.0")},
                               {QStringLiteral("profile_id"), profileId},
                               {QStringLiteral("profile_version"), QStringLiteral("1.0")},
                               {QStringLiteral("source_resolution"), QJsonObject {{QStringLiteral("width"), target.sourceResolution.width},
                                                                                    {QStringLiteral("height"), target.sourceResolution.height}}},
                               {QStringLiteral("corners"), corners},
                               {QStringLiteral("coordinate_system"), QStringLiteral("IMAGE_PIXEL")},
                               {QStringLiteral("target_coordinate_system"), QStringLiteral("SURFACE_LOCAL")},
                               {QStringLiteral("homography"), homography},
                               {QStringLiteral("inverse_homography"), inverseHomography},
                               {QStringLiteral("reprojection_error_pixels"), reprojectionErrorPixels},
                               {QStringLiteral("created_at"), QString::fromStdString(astra::common::utcTimestamp())}};
    QFile file(QDir(directory).filePath(profileId + QStringLiteral(".json")));
    if (file.open(QIODevice::WriteOnly | QIODevice::NewOnly)) file.write(QJsonDocument(profile).toJson(QJsonDocument::Compact) + '\n');
}

void SpatialService::writeAudit(const QString &event, const QString &result, int errorCode, const QJsonObject &details) const
{
    const QFileInfo info(auditPath_);
    if (!QDir().mkpath(info.dir().path())) return;
    QFile file(auditPath_);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Append)) return;
    const QJsonObject record {
        {QStringLiteral("schema_version"), QStringLiteral("1.0")},
        {QStringLiteral("event_id"), QString::fromStdString(astra::common::newUuid())},
        {QStringLiteral("timestamp"), QString::fromStdString(astra::common::utcTimestamp())},
        {QStringLiteral("service"), QStringLiteral("astra-spatial-service")},
        {QStringLiteral("event"), event},
        {QStringLiteral("trace_id"), traceId_.isEmpty() ? QString::fromStdString(astra::common::newUuid()) : traceId_},
        {QStringLiteral("request_id"), requestId_.isEmpty() ? QString::fromStdString(astra::common::newUuid()) : requestId_},
        {QStringLiteral("state"), QString::fromStdString(toString(snapshot_.state))},
        {QStringLiteral("result"), result},
        {QStringLiteral("error_code"), errorCode == 0 ? QJsonValue::Null : QJsonValue(errorCode)},
        {QStringLiteral("coordinate_system"), QString::fromStdString(toString(snapshot_.target.coordinateSystem))},
        {QStringLiteral("details"), details},
    };
    file.write(QJsonDocument(record).toJson(QJsonDocument::Compact) + '\n');
}

void SpatialService::markTargetLost()
{
    snapshot_.target.state = ProjectionTargetState::Lost;
    if (!snapshot_.sceneObject.objectId.empty()) snapshot_.sceneObject.lifecycleState = SceneObjectState::Suspended;
    setFailure(SpatialServiceState::Lost, 3305, QStringLiteral("Selected projection target was lost"));
    writeAudit(QStringLiteral("projection_target_lost"), QStringLiteral("error"), 3305);
    writeAudit(QStringLiteral("spatial_safe_pause"), QStringLiteral("success"), 3305,
               {{QStringLiteral("action"), QStringLiteral("clear_sensitive_projection_content")}});
}

} // namespace astra::spatial
