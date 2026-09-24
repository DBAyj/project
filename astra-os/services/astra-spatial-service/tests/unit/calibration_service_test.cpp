#include "vision/CalibrationService.h"

#include <cassert>
#include <cmath>
#include <limits>

using astra::spatial::CalibrationService;
using astra::spatial::CoordinateSystem;
using astra::spatial::SpatialPoint;

int main()
{
    const std::array<SpatialPoint, 4> corners {{{100.0, 100.0, CoordinateSystem::ImagePixel},
                                                  {1100.0, 100.0, CoordinateSystem::ImagePixel},
                                                  {1100.0, 620.0, CoordinateSystem::ImagePixel},
                                                  {100.0, 620.0, CoordinateSystem::ImagePixel}}};
    CalibrationService calibration;
    const auto result = calibration.calibrate(corners, {1280, 720});
    assert(result.ok);
    assert(result.value.target.state == astra::spatial::ProjectionTargetState::Calibrated);
    assert(result.value.target.targetCoordinateSystem == CoordinateSystem::SurfaceLocal);
    assert(result.value.reprojectionErrorPixels <= 3.0);

    const auto mapped = calibration.map({100.0, 100.0, CoordinateSystem::ImagePixel}, result.value.target);
    assert(mapped.ok);
    assert(std::abs(mapped.value.x) < 0.001);
    assert(std::abs(mapped.value.y) < 0.001);
    assert(mapped.value.coordinateSystem == CoordinateSystem::SurfaceLocal);

    auto selfIntersecting = corners;
    std::swap(selfIntersecting[1], selfIntersecting[2]);
    const auto invalid = calibration.calibrate(selfIntersecting, {1280, 720});
    assert(!invalid.ok);
    assert(invalid.errorCode == 3203);

    auto duplicate = corners;
    duplicate[3] = duplicate[0];
    assert(calibration.calibrate(duplicate, {1280, 720}).errorCode == 3203);

    auto outsideBounds = corners;
    outsideBounds[0].x = -1.0;
    assert(calibration.calibrate(outsideBounds, {1280, 720}).errorCode == 3202);

    auto nonFinite = corners;
    nonFinite[0].y = std::numeric_limits<double>::infinity();
    assert(calibration.calibrate(nonFinite, {1280, 720}).errorCode == 3202);

    auto unordered = corners;
    std::swap(unordered[0], unordered[1]);
    assert(calibration.calibrate(unordered, {1280, 720}).errorCode == 3203);
}
