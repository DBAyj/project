#include "application/SpatialService.h"
#include "input/FrameSource.h"
#include "vision/CalibrationService.h"
#include "vision/SurfaceDetector.h"
#include "transport/SpatialJsonRpcServer.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTemporaryDir>

#include <sys/resource.h>

#include <cassert>

namespace {

long residentSetSize()
{
    rusage usage {};
    return getrusage(RUSAGE_SELF, &usage) == 0 ? usage.ru_maxrss : -1;
}

} // namespace

int main(int argc, char **argv)
{
    QCoreApplication application(argc, argv);
    const QString root = QStringLiteral(ASTRA_SOURCE_ROOT);
    astra::spatial::FrameSourceController source(root);
    astra::spatial::SurfaceDetector detector;
    astra::spatial::CalibrationService calibration;
    assert(source.start(astra::spatial::FrameSourceType::Simulation,
                        root + QStringLiteral("/assets/spatial-fixtures/desk-front.png")).ok);
    const auto frame = source.nextFrame();
    assert(frame.ok);
    const auto candidates = detector.detect(frame.value);
    assert(candidates.ok && !candidates.value.isEmpty());
    const int frameCycles = qEnvironmentVariableIntValue("ASTRA_P3_STABILITY_FRAMES") > 0 ? qEnvironmentVariableIntValue("ASTRA_P3_STABILITY_FRAMES") : 10000;
    for (int iteration = 0; iteration < frameCycles; ++iteration) assert(detector.detect(frame.value).ok);
    for (int iteration = 0; iteration < 1000; ++iteration) assert(detector.select(candidates.value, {1280, 720}, false).ok);
    for (int iteration = 0; iteration < 500; ++iteration) assert(calibration.calibrate(candidates.value.first().corners, {1280, 720}).ok);
    for (int iteration = 0; iteration < 100; ++iteration) {
        assert(source.start(astra::spatial::FrameSourceType::Image,
                            root + QStringLiteral("/assets/spatial-fixtures/wall-front.png")).ok);
        assert(source.start(astra::spatial::FrameSourceType::Simulation,
                            root + QStringLiteral("/assets/spatial-fixtures/desk-front.png")).ok);
    }
    QTemporaryDir directory;
    astra::spatial::SpatialService service(root, directory.filePath("audit/stability.jsonl"));
    for (int iteration = 0; iteration < 100; ++iteration) {
        assert(service.start(astra::spatial::FrameSourceType::Simulation,
                             root + QStringLiteral("/assets/spatial-fixtures/desk-front.png")).ok);
        assert(service.stop().ok);
    }
    assert(source.stop().ok);
    const bool sourceReleased = !source.running();
    const bool serviceStopped = service.snapshot().state == astra::spatial::SpatialServiceState::Stopped;
    QFile audit(directory.filePath("audit/stability.jsonl"));
    const bool auditWritten = audit.exists() && audit.size() > 0;
    const QString socketPath = directory.filePath("spatial.sock");
    astra::spatial::SpatialJsonRpcServer server(&service, QStringLiteral("test-token"), {QStringLiteral("spatial.read")});
    assert(server.listen(socketPath));
    server.close();
    const bool socketReleased = !QFile::exists(socketPath);
    const long rss = residentSetSize();
    const bool resourceResidue = !(sourceReleased && serviceStopped && auditWritten && socketReleased);
    assert(!resourceResidue);
    const QString output = qEnvironmentVariable("ASTRA_P3_STABILITY_PATH", root + QStringLiteral("/runtime/tmp/p3-stability.json"));
    QDir().mkpath(QFileInfo(output).dir().path());
    QFile file(output);
    assert(file.open(QIODevice::WriteOnly | QIODevice::Truncate));
    const QJsonObject evidence {{QStringLiteral("status"), QStringLiteral("PASSED")},
                                {QStringLiteral("frame_cycles"), frameCycles},
                                {QStringLiteral("selection_cycles"), 1000},
                                {QStringLiteral("homography_cycles"), 500},
                                {QStringLiteral("source_switch_cycles"), 100},
                                {QStringLiteral("service_start_stop_cycles"), 100},
                                {QStringLiteral("audit_entries_written"), auditWritten},
                                {QStringLiteral("socket_released"), socketReleased},
                                {QStringLiteral("rss_peak_native_units"), static_cast<qint64>(rss)},
                                {QStringLiteral("resource_residue"), resourceResidue}};
    assert(file.write(QJsonDocument(evidence).toJson(QJsonDocument::Compact) + '\n') > 0);
}
