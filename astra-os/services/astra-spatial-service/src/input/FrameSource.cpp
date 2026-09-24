#include "input/FrameSource.h"

#include "astra/common/Identifiers.h"

#include <QFileInfo>
#include <QCameraDevice>
#include <QMediaDevices>

#include <opencv2/imgcodecs.hpp>
#include <opencv2/videoio.hpp>

namespace astra::spatial {
namespace {

SpatialResult<bool> failure(int code, const QString &message)
{
    return {false, false, code, message};
}

} // namespace

FrameSourceController::FrameSourceController(QString projectRoot) : projectRoot_(std::move(projectRoot)) {}

QVector<FrameSourceDescriptor> FrameSourceController::enumerate() const
{
    QVector<FrameSourceDescriptor> values {
        {FrameSourceType::Simulation, QStringLiteral("simulation"), QStringLiteral("Deterministic simulation"), true},
        {FrameSourceType::Image, QStringLiteral("image"), QStringLiteral("Image fixture"), true},
        {FrameSourceType::Video, QStringLiteral("video"), QStringLiteral("Video file"), true},
    };
    const auto cameras = QMediaDevices::videoInputs();
    for (const auto &camera : cameras) {
        values.append({FrameSourceType::Camera, camera.id(), camera.description(), !camera.isNull()});
    }
    return values;
}

SpatialResult<bool> FrameSourceController::start(FrameSourceType source, const QString &location)
{
    QString resolved = location;
    if (source == FrameSourceType::Simulation && resolved.isEmpty()) resolved = defaultFixturePath();
    if ((source == FrameSourceType::Simulation || source == FrameSourceType::Image || source == FrameSourceType::Video)
        && (resolved.isEmpty() || !QFileInfo::exists(resolved))) {
        return failure(3004, QStringLiteral("Spatial input source is unavailable"));
    }
    if (source == FrameSourceType::Camera && QMediaDevices::videoInputs().isEmpty()) {
        return failure(3005, QStringLiteral("Camera is unavailable"));
    }
    source_ = source;
    location_ = resolved;
    frameCounter_ = 0;
    running_ = true;
    return {true, true, 0, {}};
}

SpatialResult<FramePacket> FrameSourceController::nextFrame()
{
    if (!running_) return {false, {}, 3001, QStringLiteral("Spatial service is not started")};
    if (source_ == FrameSourceType::Simulation || source_ == FrameSourceType::Image) return loadImage(location_);

    cv::VideoCapture capture;
    if (source_ == FrameSourceType::Camera) {
        capture.open(0);
    } else {
        capture.open(location_.toStdString());
    }
    cv::Mat pixels;
    if (!capture.isOpened() || !capture.read(pixels) || pixels.empty()) {
        return {false, {}, source_ == FrameSourceType::Camera ? 3005 : 3009, QStringLiteral("Spatial input source is unavailable")};
    }
    ++frameCounter_;
    return {true,
            {{astra::common::newUuid(), source_, {pixels.cols, pixels.rows}, "BGR8", CoordinateSystem::ImagePixel,
              astra::common::utcTimestamp()},
             pixels},
            0,
            {}};
}

SpatialResult<bool> FrameSourceController::stop()
{
    running_ = false;
    location_.clear();
    frameCounter_ = 0;
    return {true, true, 0, {}};
}

bool FrameSourceController::running() const { return running_; }
FrameSourceType FrameSourceController::activeSource() const { return source_; }

QString FrameSourceController::defaultFixturePath() const
{
    return projectRoot_ + QStringLiteral("/assets/spatial-fixtures/desk-front.png");
}

SpatialResult<FramePacket> FrameSourceController::loadImage(const QString &path) const
{
    cv::Mat pixels = cv::imread(path.toStdString(), cv::IMREAD_COLOR);
    if (pixels.empty()) return {false, {}, 3008, QStringLiteral("Spatial image frame is invalid")};
    return {true,
            {{astra::common::newUuid(), source_, {pixels.cols, pixels.rows}, "BGR8", CoordinateSystem::ImagePixel,
              astra::common::utcTimestamp()},
             pixels},
            0,
            {}};
}

} // namespace astra::spatial
