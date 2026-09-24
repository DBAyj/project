#pragma once

#include "input/FrameSource.h"

#include <array>

namespace astra::spatial {

struct CalibrationResult {
    ProjectionTarget target;
    double reprojectionErrorPixels {0.0};
};

class CalibrationService final {
public:
    SpatialResult<CalibrationResult> calibrate(const std::array<SpatialPoint, 4> &corners,
                                                Resolution sourceResolution) const;
    SpatialResult<SpatialPoint> map(const SpatialPoint &point, const ProjectionTarget &target) const;
};

} // namespace astra::spatial
