#include "application/SpatialService.h"

#include <QCoreApplication>
#include <QFile>
#include <QTemporaryDir>

#include <cassert>

int main(int argc, char **argv)
{
    QCoreApplication application(argc, argv);
    QTemporaryDir directory;
    const QString root = QStringLiteral(ASTRA_SOURCE_ROOT);
    const QString audit = directory.filePath("audit/spatial-audit.jsonl");
    astra::spatial::SpatialService service(root, audit);

    assert(service.start(astra::spatial::FrameSourceType::Simulation,
                         root + QStringLiteral("/assets/spatial-fixtures/desk-front.png")).ok);
    assert(service.snapshot().state == astra::spatial::SpatialServiceState::Ready);
    assert(service.detect().ok);
    assert(!service.snapshot().candidates.isEmpty());
    assert(service.select(false).ok);
    assert(service.calibrate(service.snapshot().target.corners).ok);
    assert(service.snapshot().state == astra::spatial::SpatialServiceState::Tracking);
    assert(service.snapshot().target.state == astra::spatial::ProjectionTargetState::Calibrated);
    assert(!service.snapshot().anchor.anchorId.empty());
    assert(service.snapshot().target.privacyLevel == "NO_PROJECTION");
    assert(service.snapshot().sceneObject.lifecycleState == astra::spatial::SceneObjectState::Active);

    astra::spatial::FrameSourceController availability(root);
    bool cameraAvailable = false;
    for (const auto &descriptor : availability.enumerate()) {
        cameraAvailable = cameraAvailable || (descriptor.type == astra::spatial::FrameSourceType::Camera && descriptor.available);
    }
    if (!cameraAvailable) {
        assert(service.start(astra::spatial::FrameSourceType::Camera).ok);
        assert(service.snapshot().inputSource == astra::spatial::FrameSourceType::Simulation);
        assert(service.snapshot().warning.contains(QStringLiteral("fallback")));
    }

    assert(service.start(astra::spatial::FrameSourceType::Simulation,
                         root + QStringLiteral("/assets/spatial-fixtures/no-surface.png")).ok);
    assert(service.detect().ok);
    assert(service.snapshot().state == astra::spatial::SpatialServiceState::Lost);
    assert(service.snapshot().lastErrorCode == 3305);
    assert(service.snapshot().target.state == astra::spatial::ProjectionTargetState::Lost);

    QFile auditFile(audit);
    assert(auditFile.open(QIODevice::ReadOnly));
    const QByteArray auditContent = auditFile.readAll();
    assert(auditContent.contains("spatial_safe_pause"));
    assert(!auditContent.contains("raw_frame"));
    assert(!auditContent.contains("image_base64"));
    assert(service.stop().ok);
    assert(service.snapshot().state == astra::spatial::SpatialServiceState::Stopped);
}
