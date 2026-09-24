#include "astra/projection/ProjectionRuntime.h"

#include "astra/common/ErrorCode.h"
#include "astra/projection/SafeClearController.h"
#include "astra/render/RenderGraph.h"

namespace astra::projection {

ProjectionRuntime::ProjectionRuntime(astra::render::ProjectionOutput &output, ProjectionSession &session)
    : output_(output)
    , session_(session)
{
}

ProjectionRenderResult ProjectionRuntime::render(const ProjectionRenderRequest &request)
{
    if (request.p3IntegrationStatus != QString::fromLatin1(kP3FixtureOnlyMarker)
        && request.p3IntegrationStatus != QString::fromLatin1(kP3ServiceTargetVerifiedMarker)) {
        SafeClearController::clear(output_, session_, SafeClearReason::ServiceException);
        return {false, static_cast<int>(astra::common::ErrorCode::ProjectionRenderPassFailed), 0};
    }
    if (request.privacyLevel == astra::common::PrivacyLevel::NoProjection
        || request.privacyLevel == astra::common::PrivacyLevel::PrivateScreenOnly) {
        const auto result = SafeClearController::clear(output_, session_, SafeClearReason::NoProjection);
        return {result.ok, result.errorCode, 0};
    }
    for (const auto &layer : request.layers) {
        if (!layer.visible) continue;
        if (layer.privacyLevel == astra::common::PrivacyLevel::NoProjection
            || layer.privacyLevel == astra::common::PrivacyLevel::PrivateScreenOnly) {
            static_cast<void>(SafeClearController::clear(output_, session_, SafeClearReason::NoProjection));
            return {false, static_cast<int>(astra::common::ErrorCode::ProjectionRenderPassFailed), 0};
        }
    }
    if (request.input.isNull()) {
        SafeClearController::clear(output_, session_, SafeClearReason::RenderPassFailed);
        return {false, static_cast<int>(astra::common::ErrorCode::ProjectionRenderPassFailed), 0};
    }
    const auto homography = astra::render::Homography::fromUnitSquare(request.corners, output_.resolution());
    if (!homography.ok) {
        SafeClearController::clear(output_, session_, SafeClearReason::HomographyFailed);
        return {false, homography.errorCode, 0};
    }
    if (session_.state() == SessionState::Ready && !session_.startRendering().ok) {
        SafeClearController::clear(output_, session_, SafeClearReason::RenderPassFailed);
        return {false, static_cast<int>(astra::common::ErrorCode::ProjectionRenderPassFailed), 0};
    }
    if (session_.state() != SessionState::Rendering) {
        SafeClearController::clear(output_, session_, SafeClearReason::RenderPassFailed);
        return {false, static_cast<int>(astra::common::ErrorCode::ProjectionRenderPassFailed), 0};
    }

    static_cast<void>(astra::render::compositionOrder(request.layers));
    QImage offscreen = request.input.convertToFormat(QImage::Format_RGBA8888);
    const auto resolution = output_.resolution();
    if (offscreen.width() != resolution.width || offscreen.height() != resolution.height) {
        offscreen = offscreen.scaled(resolution.width, resolution.height, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
    }

    const auto graph = astra::render::RenderGraph::fixedP4Pipeline();
    const bool outputAvailable = output_.state() != astra::render::OutputState::Error && output_.state() != astra::render::OutputState::Stopped;
    const auto graphResult = graph.run(outputAvailable, [&](const astra::render::RenderPass &pass) {
        if (pass.id == QStringLiteral("CropAndOverscanPass")) {
            const auto crop = astra::render::CropOverscanPass::apply(offscreen, request.cropOverscan);
            if (!crop.ok) return false;
            offscreen = crop.frame;
            return true;
        }
        if (pass.id == QStringLiteral("GeometryWarpPass")) {
            offscreen = astra::render::Homography::warp(offscreen, homography.inverse, QSize {resolution.width, resolution.height});
            if (offscreen.isNull()) return false;
            if (!request.meshWarp.has_value()) return true;
            const auto mesh = astra::render::MeshWarp::apply(offscreen, *request.meshWarp, QSize {resolution.width, resolution.height});
            if (!mesh.ok) return false;
            offscreen = mesh.frame;
            return true;
        }
        if (pass.id == QStringLiteral("ColorCompensationPass")) {
            return astra::render::ColorCompensationPass::apply(offscreen, request.colorCompensation).ok;
        }
        if (pass.id == QStringLiteral("PrivacyMaskPass") && request.privacyMask.has_value()) {
            return astra::render::PrivacyMaskPass::apply(offscreen, *request.privacyMask).ok;
        }
        if (pass.id == QStringLiteral("OutputPass")) return output_.present(offscreen).ok;
        return true;
    });
    if (!graphResult.ok) {
        SafeClearController::clear(output_, session_, SafeClearReason::RenderPassFailed);
        return {false, graphResult.errorCode, 0};
    }
    return {true, 0, nextFrameId_++};
}

} // namespace astra::projection
