#include "domain/SpatialTypes.h"

#include <utility>

namespace astra::spatial {
namespace {

template <typename T>
std::optional<T> lookup(const std::string &value, std::initializer_list<std::pair<const char *, T>> values)
{
    for (const auto &[name, item] : values) {
        if (value == name) return item;
    }
    return std::nullopt;
}

} // namespace

std::string toString(FrameSourceType value)
{
    switch (value) {
    case FrameSourceType::Camera: return "CAMERA";
    case FrameSourceType::Image: return "IMAGE";
    case FrameSourceType::Video: return "VIDEO";
    case FrameSourceType::Simulation: return "SIMULATION";
    }
    return "SIMULATION";
}

std::string toString(CoordinateSystem value)
{
    switch (value) {
    case CoordinateSystem::ImagePixel: return "IMAGE_PIXEL";
    case CoordinateSystem::ImageNormalized: return "IMAGE_NORMALIZED";
    case CoordinateSystem::PhoneView: return "PHONE_VIEW";
    case CoordinateSystem::ProjectionView: return "PROJECTION_VIEW";
    case CoordinateSystem::SurfaceLocal: return "SURFACE_LOCAL";
    case CoordinateSystem::WorldSimulated: return "WORLD_SIMULATED";
    }
    return "IMAGE_PIXEL";
}

std::string toString(SurfaceType value)
{
    switch (value) {
    case SurfaceType::Desk: return "DESK";
    case SurfaceType::Wall: return "WALL";
    case SurfaceType::Screen: return "SCREEN";
    case SurfaceType::Unknown: return "UNKNOWN";
    }
    return "UNKNOWN";
}

std::string toString(SpatialQuality value)
{
    switch (value) {
    case SpatialQuality::Excellent: return "EXCELLENT";
    case SpatialQuality::Good: return "GOOD";
    case SpatialQuality::Fair: return "FAIR";
    case SpatialQuality::Poor: return "POOR";
    case SpatialQuality::Unusable: return "UNUSABLE";
    }
    return "UNUSABLE";
}

std::string toString(SpatialServiceState value)
{
    switch (value) {
    case SpatialServiceState::Stopped: return "STOPPED";
    case SpatialServiceState::Starting: return "STARTING";
    case SpatialServiceState::Ready: return "READY";
    case SpatialServiceState::Capturing: return "CAPTURING";
    case SpatialServiceState::Detecting: return "DETECTING";
    case SpatialServiceState::Tracking: return "TRACKING";
    case SpatialServiceState::Degraded: return "DEGRADED";
    case SpatialServiceState::Lost: return "LOST";
    case SpatialServiceState::Stopping: return "STOPPING";
    case SpatialServiceState::Error: return "ERROR";
    }
    return "ERROR";
}

std::string toString(SceneState value)
{
    switch (value) {
    case SceneState::Empty: return "EMPTY";
    case SceneState::Initializing: return "INITIALIZING";
    case SceneState::Tracking: return "TRACKING";
    case SceneState::Degraded: return "DEGRADED";
    case SceneState::Lost: return "LOST";
    case SceneState::Stopped: return "STOPPED";
    case SceneState::Error: return "ERROR";
    }
    return "ERROR";
}

std::string toString(SceneObjectState value)
{
    switch (value) {
    case SceneObjectState::Created: return "CREATED";
    case SceneObjectState::Active: return "ACTIVE";
    case SceneObjectState::Hidden: return "HIDDEN";
    case SceneObjectState::Suspended: return "SUSPENDED";
    case SceneObjectState::Removed: return "REMOVED";
    case SceneObjectState::Error: return "ERROR";
    }
    return "ERROR";
}

std::string toString(ProjectionTargetState value)
{
    switch (value) {
    case ProjectionTargetState::Candidate: return "CANDIDATE";
    case ProjectionTargetState::Selected: return "SELECTED";
    case ProjectionTargetState::Calibrated: return "CALIBRATED";
    case ProjectionTargetState::Active: return "ACTIVE";
    case ProjectionTargetState::Degraded: return "DEGRADED";
    case ProjectionTargetState::Lost: return "LOST";
    case ProjectionTargetState::Invalid: return "INVALID";
    }
    return "INVALID";
}

std::optional<FrameSourceType> frameSourceTypeFromString(const std::string &value)
{
    return lookup<FrameSourceType>(value, {{"CAMERA", FrameSourceType::Camera}, {"IMAGE", FrameSourceType::Image},
                                          {"VIDEO", FrameSourceType::Video}, {"SIMULATION", FrameSourceType::Simulation}});
}

std::optional<CoordinateSystem> coordinateSystemFromString(const std::string &value)
{
    return lookup<CoordinateSystem>(value, {{"IMAGE_PIXEL", CoordinateSystem::ImagePixel},
                                            {"IMAGE_NORMALIZED", CoordinateSystem::ImageNormalized},
                                            {"PHONE_VIEW", CoordinateSystem::PhoneView},
                                            {"PROJECTION_VIEW", CoordinateSystem::ProjectionView},
                                            {"SURFACE_LOCAL", CoordinateSystem::SurfaceLocal},
                                            {"WORLD_SIMULATED", CoordinateSystem::WorldSimulated}});
}

int qualityRank(SpatialQuality value)
{
    switch (value) {
    case SpatialQuality::Excellent: return 4;
    case SpatialQuality::Good: return 3;
    case SpatialQuality::Fair: return 2;
    case SpatialQuality::Poor: return 1;
    case SpatialQuality::Unusable: return 0;
    }
    return 0;
}

bool isAutoSelectable(SpatialQuality value)
{
    return value == SpatialQuality::Excellent || value == SpatialQuality::Good;
}

bool isValidSpatialTransition(SpatialServiceState from, SpatialServiceState to)
{
    using State = SpatialServiceState;
    switch (from) {
    case State::Stopped: return to == State::Starting;
    case State::Starting: return to == State::Ready || to == State::Error;
    case State::Ready: return to == State::Capturing || to == State::Stopping;
    case State::Capturing: return to == State::Detecting || to == State::Stopping;
    case State::Detecting: return to == State::Tracking || to == State::Degraded || to == State::Lost;
    case State::Tracking: return to == State::Degraded || to == State::Lost || to == State::Stopping;
    case State::Degraded: return to == State::Tracking || to == State::Lost || to == State::Stopping;
    case State::Lost: return to == State::Detecting || to == State::Stopping;
    case State::Stopping: return to == State::Stopped;
    case State::Error: return to == State::Stopped;
    }
    return false;
}

} // namespace astra::spatial
