#pragma once

#include "domain/SpatialTypes.h"

#include <opencv2/core/mat.hpp>

#include <QString>
#include <QVector>

namespace astra::spatial {

struct FramePacket {
    SpatialFrame frame;
    cv::Mat pixels;
};

struct FrameSourceDescriptor {
    FrameSourceType type {FrameSourceType::Simulation};
    QString identifier;
    QString displayName;
    bool available {false};
};

template <typename T>
struct SpatialResult {
    bool ok {false};
    T value {};
    int errorCode {0};
    QString message;
};

class FrameSourceController final {
public:
    explicit FrameSourceController(QString projectRoot);

    QVector<FrameSourceDescriptor> enumerate() const;
    SpatialResult<bool> start(FrameSourceType source, const QString &location = {});
    SpatialResult<FramePacket> nextFrame();
    SpatialResult<bool> stop();
    bool running() const;
    FrameSourceType activeSource() const;

private:
    QString defaultFixturePath() const;
    SpatialResult<FramePacket> loadImage(const QString &path) const;

    QString projectRoot_;
    QString location_;
    FrameSourceType source_ {FrameSourceType::Simulation};
    bool running_ {false};
    quint64 frameCounter_ {0};
};

} // namespace astra::spatial
