#include "domain/SpatialTypes.h"

#include <cassert>

using namespace astra::spatial;

int main()
{
    assert(frameSourceTypeFromString("CAMERA") == FrameSourceType::Camera);
    assert(frameSourceTypeFromString("IMAGE") == FrameSourceType::Image);
    assert(frameSourceTypeFromString("VIDEO") == FrameSourceType::Video);
    assert(frameSourceTypeFromString("SIMULATION") == FrameSourceType::Simulation);
    assert(!frameSourceTypeFromString("UNKNOWN_SOURCE").has_value());

    assert(coordinateSystemFromString("IMAGE_PIXEL") == CoordinateSystem::ImagePixel);
    assert(coordinateSystemFromString("IMAGE_NORMALIZED") == CoordinateSystem::ImageNormalized);
    assert(coordinateSystemFromString("PHONE_VIEW") == CoordinateSystem::PhoneView);
    assert(coordinateSystemFromString("PROJECTION_VIEW") == CoordinateSystem::ProjectionView);
    assert(coordinateSystemFromString("SURFACE_LOCAL") == CoordinateSystem::SurfaceLocal);
    assert(coordinateSystemFromString("WORLD_SIMULATED") == CoordinateSystem::WorldSimulated);
    assert(!coordinateSystemFromString("IMPLICIT").has_value());

    assert(qualityRank(SpatialQuality::Excellent) > qualityRank(SpatialQuality::Good));
    assert(qualityRank(SpatialQuality::Good) > qualityRank(SpatialQuality::Fair));
    assert(isAutoSelectable(SpatialQuality::Excellent));
    assert(isAutoSelectable(SpatialQuality::Good));
    assert(!isAutoSelectable(SpatialQuality::Fair));
    assert(ProjectionTarget {}.privacyLevel == "NO_PROJECTION");

    assert(isValidSpatialTransition(SpatialServiceState::Stopped, SpatialServiceState::Starting));
    assert(isValidSpatialTransition(SpatialServiceState::Starting, SpatialServiceState::Ready));
    assert(isValidSpatialTransition(SpatialServiceState::Detecting, SpatialServiceState::Tracking));
    assert(isValidSpatialTransition(SpatialServiceState::Tracking, SpatialServiceState::Lost));
    assert(isValidSpatialTransition(SpatialServiceState::Stopping, SpatialServiceState::Stopped));
    assert(!isValidSpatialTransition(SpatialServiceState::Stopped, SpatialServiceState::Tracking));
    assert(!isValidSpatialTransition(SpatialServiceState::Lost, SpatialServiceState::Ready));

    const SpatialPoint point {100.0, 200.0, CoordinateSystem::ImagePixel};
    assert(point.coordinateSystem == CoordinateSystem::ImagePixel);
    assert(toString(point.coordinateSystem) == "IMAGE_PIXEL");
}
