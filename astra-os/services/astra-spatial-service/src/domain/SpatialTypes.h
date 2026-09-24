#pragma once

#include <array>
#include <optional>
#include <string>
#include <vector>

namespace astra::spatial {

enum class FrameSourceType { Camera, Image, Video, Simulation };
enum class CoordinateSystem { ImagePixel, ImageNormalized, PhoneView, ProjectionView, SurfaceLocal, WorldSimulated };
enum class SurfaceType { Desk, Wall, Screen, Unknown };
enum class SpatialQuality { Excellent, Good, Fair, Poor, Unusable };
enum class SpatialServiceState { Stopped, Starting, Ready, Capturing, Detecting, Tracking, Degraded, Lost, Stopping, Error };
enum class SceneState { Empty, Initializing, Tracking, Degraded, Lost, Stopped, Error };
enum class SceneObjectState { Created, Active, Hidden, Suspended, Removed, Error };
enum class ProjectionTargetState { Candidate, Selected, Calibrated, Active, Degraded, Lost, Invalid };

std::string toString(FrameSourceType value);
std::string toString(CoordinateSystem value);
std::string toString(SurfaceType value);
std::string toString(SpatialQuality value);
std::string toString(SpatialServiceState value);
std::string toString(SceneState value);
std::string toString(SceneObjectState value);
std::string toString(ProjectionTargetState value);

std::optional<FrameSourceType> frameSourceTypeFromString(const std::string &value);
std::optional<CoordinateSystem> coordinateSystemFromString(const std::string &value);
int qualityRank(SpatialQuality value);
bool isAutoSelectable(SpatialQuality value);
bool isValidSpatialTransition(SpatialServiceState from, SpatialServiceState to);

struct SpatialPoint {
    double x {0.0};
    double y {0.0};
    CoordinateSystem coordinateSystem {CoordinateSystem::ImagePixel};
};

struct Vector3 {
    double x {0.0};
    double y {0.0};
    double z {0.0};
};

struct Resolution {
    int width {0};
    int height {0};
};

struct SpatialFrame {
    std::string frameId;
    FrameSourceType source {FrameSourceType::Simulation};
    Resolution resolution;
    std::string pixelFormat {"BGR8"};
    CoordinateSystem coordinateSystem {CoordinateSystem::ImagePixel};
    std::string timestamp;
};

struct SurfaceCandidate {
    std::string surfaceId;
    SurfaceType surfaceType {SurfaceType::Unknown};
    double confidence {0.0};
    std::array<SpatialPoint, 4> corners;
    double areaRatio {0.0};
    double aspectRatio {0.0};
    double stabilityScore {0.0};
    double score {0.0};
    SpatialQuality quality {SpatialQuality::Unusable};
};

struct SpatialAnchor {
    std::string anchorId;
    std::string anchorType;
    CoordinateSystem coordinateSystem {CoordinateSystem::SurfaceLocal};
    std::array<double, 16> transform {};
    SpatialQuality quality {SpatialQuality::Unusable};
    bool persistent {false};
    std::string createdAt;
    std::string lastUpdatedAt;
};

struct SceneObject {
    std::string objectId;
    std::string sceneId;
    std::string objectType;
    std::string modelId;
    std::string anchorId;
    CoordinateSystem coordinateSystem {CoordinateSystem::SurfaceLocal};
    Vector3 position;
    Vector3 rotation;
    Vector3 scale {1.0, 1.0, 1.0};
    bool visible {true};
    std::string privacyLevel {"NO_PROJECTION"};
    bool interactionEnabled {true};
    SceneObjectState lifecycleState {SceneObjectState::Created};
};

struct SpatialScene {
    std::string sceneId;
    SceneState state {SceneState::Empty};
    CoordinateSystem coordinateSystem {CoordinateSystem::WorldSimulated};
    std::vector<std::string> anchorIds;
    std::vector<std::string> objectIds;
    std::string createdAt;
    std::string updatedAt;
};

struct ProjectionTarget {
    std::string targetId;
    std::string surfaceId;
    SurfaceType surfaceType {SurfaceType::Unknown};
    Resolution sourceResolution;
    Resolution targetResolution;
    std::array<SpatialPoint, 4> corners;
    CoordinateSystem coordinateSystem {CoordinateSystem::ImagePixel};
    CoordinateSystem targetCoordinateSystem {CoordinateSystem::SurfaceLocal};
    std::array<double, 9> homography {};
    std::array<double, 9> inverseHomography {};
    SpatialQuality quality {SpatialQuality::Unusable};
    std::string privacyLevel {"NO_PROJECTION"};
    ProjectionTargetState state {ProjectionTargetState::Candidate};
    bool selected {false};
    std::string createdAt;
    std::string updatedAt;
};

struct ObserverPose {
    std::string observerId {"primary-observer"};
    Vector3 position {0.0, 0.2, 1.5};
    Vector3 orientation;
    std::string trackingState {"TRACKING"};
    double confidence {1.0};
    CoordinateSystem coordinateSystem {CoordinateSystem::WorldSimulated};
};

} // namespace astra::spatial
