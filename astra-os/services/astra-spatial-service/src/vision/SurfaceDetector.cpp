#include "vision/SurfaceDetector.h"

#include "astra/common/Identifiers.h"

#include <opencv2/imgproc.hpp>
#include <opencv2/geometry/2d.hpp>

#include <algorithm>
#include <array>
#include <cmath>
#include <vector>

namespace astra::spatial {
namespace {

std::array<cv::Point2f, 4> orderCorners(const std::vector<cv::Point> &points)
{
    std::array<cv::Point2f, 4> ordered {};
    std::array<float, 4> sums {};
    std::array<float, 4> differences {};
    for (std::size_t index = 0; index < points.size(); ++index) {
        const auto &point = points[index];
        sums[index] = static_cast<float>(point.x + point.y);
        differences[index] = static_cast<float>(point.x - point.y);
    }
    const auto minSum = std::min_element(sums.begin(), sums.end()) - sums.begin();
    const auto maxSum = std::max_element(sums.begin(), sums.end()) - sums.begin();
    const auto maxDifference = std::max_element(differences.begin(), differences.end()) - differences.begin();
    const auto minDifference = std::min_element(differences.begin(), differences.end()) - differences.begin();
    ordered[0] = points[static_cast<std::size_t>(minSum)];
    ordered[1] = points[static_cast<std::size_t>(maxDifference)];
    ordered[2] = points[static_cast<std::size_t>(maxSum)];
    ordered[3] = points[static_cast<std::size_t>(minDifference)];
    return ordered;
}

SpatialQuality qualityFor(double score)
{
    if (score >= 0.85) return SpatialQuality::Excellent;
    if (score >= 0.70) return SpatialQuality::Good;
    if (score >= 0.55) return SpatialQuality::Fair;
    if (score >= 0.35) return SpatialQuality::Poor;
    return SpatialQuality::Unusable;
}

std::array<double, 9> identityMatrix()
{
    return {1.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 1.0};
}

} // namespace

SurfaceDetector::SurfaceDetector(DetectionConfig config) : config_(config) {}

SpatialResult<cv::Mat> SurfaceDetector::preprocess(const FramePacket &frame) const
{
    if (frame.pixels.empty() || frame.frame.coordinateSystem != CoordinateSystem::ImagePixel) {
        return {false, {}, 3008, QStringLiteral("Spatial image frame is invalid")};
    }
    cv::Mat resized;
    cv::resize(frame.pixels, resized, {config_.targetWidth, config_.targetHeight}, 0.0, 0.0, cv::INTER_AREA);
    cv::Mat grayscale;
    cv::cvtColor(resized, grayscale, cv::COLOR_BGR2GRAY);
    cv::Mat blurred;
    cv::GaussianBlur(grayscale, blurred, {config_.gaussianBlurKernel, config_.gaussianBlurKernel}, 0.0);
    cv::Mat edges;
    cv::Canny(blurred, edges, config_.cannyLowThreshold, config_.cannyHighThreshold);
    return {true, edges, 0, {}};
}

SpatialResult<QVector<SurfaceCandidate>> SurfaceDetector::detect(const FramePacket &frame) const
{
    const auto processed = preprocess(frame);
    if (!processed.ok) return {false, {}, processed.errorCode, processed.message};

    std::vector<std::vector<cv::Point>> contours;
    cv::findContours(processed.value.clone(), contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);
    QVector<SurfaceCandidate> candidates;
    const double frameArea = static_cast<double>(config_.targetWidth) * config_.targetHeight;
    const cv::Point2f frameCenter {config_.targetWidth / 2.0F, config_.targetHeight / 2.0F};
    const double maxDistance = std::hypot(frameCenter.x, frameCenter.y);

    for (const auto &contour : contours) {
        const double area = std::abs(cv::contourArea(contour));
        const double areaRatio = area / frameArea;
        if (areaRatio < config_.minimumAreaRatio || areaRatio > config_.maximumAreaRatio) continue;

        std::vector<cv::Point> polygon;
        cv::approxPolyDP(contour, polygon, config_.polygonEpsilonRatio * cv::arcLength(contour, true), true);
        if (polygon.size() != 4 || !cv::isContourConvex(polygon)) continue;

        const auto corners = orderCorners(polygon);
        const double top = cv::norm(corners[1] - corners[0]);
        const double bottom = cv::norm(corners[2] - corners[3]);
        const double left = cv::norm(corners[3] - corners[0]);
        const double right = cv::norm(corners[2] - corners[1]);
        const double height = (left + right) / 2.0;
        if (height <= 0.0) continue;
        const double aspect = (top + bottom) / (2.0 * height);
        if (aspect < config_.minimumAspectRatio || aspect > config_.maximumAspectRatio) continue;

        const auto moments = cv::moments(polygon);
        if (moments.m00 == 0.0) continue;
        const cv::Point2f center {static_cast<float>(moments.m10 / moments.m00), static_cast<float>(moments.m01 / moments.m00)};
        const double geometry = std::max(0.0, 1.0 - std::abs(aspect - 1.6) / 1.6);
        const double areaScore = std::min(1.0, areaRatio / 0.40);
        const double position = std::max(0.0, 1.0 - cv::norm(center - frameCenter) / maxDistance);
        const double score = geometry * 0.30 + areaScore * 0.20 + 0.25 + position * 0.10 + 0.12;

        SurfaceCandidate candidate;
        candidate.surfaceId = astra::common::newUuid();
        candidate.surfaceType = SurfaceType::Desk;
        candidate.confidence = score;
        candidate.corners = {{{corners[0].x, corners[0].y, CoordinateSystem::ImagePixel},
                              {corners[1].x, corners[1].y, CoordinateSystem::ImagePixel},
                              {corners[2].x, corners[2].y, CoordinateSystem::ImagePixel},
                              {corners[3].x, corners[3].y, CoordinateSystem::ImagePixel}}};
        candidate.areaRatio = areaRatio;
        candidate.aspectRatio = aspect;
        candidate.stabilityScore = 1.0;
        candidate.score = score;
        candidate.quality = qualityFor(score);
        candidates.append(candidate);
    }
    std::sort(candidates.begin(), candidates.end(), [](const SurfaceCandidate &left, const SurfaceCandidate &right) {
        return left.score > right.score;
    });
    return {true, candidates, 0, {}};
}

SpatialResult<ProjectionTarget> SurfaceDetector::select(const QVector<SurfaceCandidate> &candidates,
                                                         Resolution sourceResolution,
                                                         bool manualConfirmation) const
{
    for (const auto &candidate : candidates) {
        const bool eligible = manualConfirmation ? qualityRank(candidate.quality) >= qualityRank(SpatialQuality::Fair)
                                                  : isAutoSelectable(candidate.quality);
        if (!eligible) continue;
        ProjectionTarget target;
        target.targetId = astra::common::newUuid();
        target.surfaceId = candidate.surfaceId;
        target.surfaceType = candidate.surfaceType;
        target.sourceResolution = sourceResolution;
        target.targetResolution = sourceResolution;
        target.corners = candidate.corners;
        target.coordinateSystem = CoordinateSystem::ImagePixel;
        target.targetCoordinateSystem = CoordinateSystem::SurfaceLocal;
        target.homography = identityMatrix();
        target.inverseHomography = identityMatrix();
        target.quality = candidate.quality;
        target.state = ProjectionTargetState::Selected;
        target.selected = true;
        target.createdAt = astra::common::utcTimestamp();
        target.updatedAt = target.createdAt;
        return {true, target, 0, {}};
    }
    const int errorCode = candidates.isEmpty() ? 3101 : 3102;
    return {false, {}, errorCode, QStringLiteral("No policy-eligible surface was detected")};
}

} // namespace astra::spatial
