#pragma once

#include "astra/common/PrivacyLevel.h"
#include "astra/projection/ProjectionSession.h"
#include "astra/render/Homography.h"
#include "astra/render/ColorCompensation.h"
#include "astra/render/CropOverscan.h"
#include "astra/render/MeshWarp.h"
#include "astra/render/PrivacyMask.h"
#include "astra/render/ProjectionLayer.h"
#include "astra/render/ProjectionOutput.h"

#include <QImage>

#include <array>
#include <optional>

namespace astra::projection {

inline constexpr const char *kP3FixtureOnlyMarker = "P3_REAL_SPATIAL_INTEGRATION_NOT_VERIFIED";
inline constexpr const char *kP3ServiceTargetVerifiedMarker = "P3_SERVICE_PROJECTION_TARGET_VERIFIED";

struct ProjectionRenderRequest {
    QImage input;
    astra::common::PrivacyLevel privacyLevel {astra::common::PrivacyLevel::NoProjection};
    QVector<astra::render::ProjectionLayer> layers;
    std::array<astra::render::Point, 4> corners {};
    astra::render::CropOverscanSettings cropOverscan;
    astra::render::ColorCompensationSettings colorCompensation;
    std::optional<astra::render::MeshWarpSettings> meshWarp;
    std::optional<astra::render::PrivacyMask> privacyMask;
    QString p3IntegrationStatus {QString::fromLatin1(kP3FixtureOnlyMarker)};
};

struct ProjectionRenderResult {
    bool ok {false};
    int errorCode {0};
    quint64 frameId {0};
};

class ProjectionRuntime final {
public:
    ProjectionRuntime(astra::render::ProjectionOutput &output, ProjectionSession &session);

    ProjectionRenderResult render(const ProjectionRenderRequest &request);

private:
    astra::render::ProjectionOutput &output_;
    ProjectionSession &session_;
    quint64 nextFrameId_ {1};
};

} // namespace astra::projection
