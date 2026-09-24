#pragma once

#include "input/FrameSource.h"

#include <opencv2/core/mat.hpp>

namespace astra::spatial {

struct DetectionConfig {
    int targetWidth {1280};
    int targetHeight {720};
    int gaussianBlurKernel {5};
    int cannyLowThreshold {50};
    int cannyHighThreshold {150};
    double minimumAreaRatio {0.10};
    double maximumAreaRatio {0.90};
    double minimumAspectRatio {0.50};
    double maximumAspectRatio {3.00};
    double polygonEpsilonRatio {0.02};
};

class SurfaceDetector final {
public:
    explicit SurfaceDetector(DetectionConfig config = {});

    SpatialResult<cv::Mat> preprocess(const FramePacket &frame) const;
    SpatialResult<QVector<SurfaceCandidate>> detect(const FramePacket &frame) const;
    SpatialResult<ProjectionTarget> select(const QVector<SurfaceCandidate> &candidates,
                                           Resolution sourceResolution,
                                           bool manualConfirmation) const;

private:
    DetectionConfig config_;
};

} // namespace astra::spatial
