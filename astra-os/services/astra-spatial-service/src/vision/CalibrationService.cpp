#include "vision/CalibrationService.h"

#include "astra/common/Identifiers.h"

#include <opencv2/calib3d.hpp>

#include <cmath>
#include <limits>
#include <vector>

namespace astra::spatial {
namespace {

double polygonArea(const std::array<SpatialPoint, 4> &points)
{
    double area = 0.0;
    for (std::size_t index = 0; index < points.size(); ++index) {
        const auto &current = points[index];
        const auto &following = points[(index + 1) % points.size()];
        area += current.x * following.y - current.y * following.x;
    }
    return std::abs(area) / 2.0;
}

double cross(const SpatialPoint &first, const SpatialPoint &second, const SpatialPoint &third)
{
    return (second.x - first.x) * (third.y - first.y) - (second.y - first.y) * (third.x - first.x);
}

bool intersects(const SpatialPoint &first, const SpatialPoint &second, const SpatialPoint &third, const SpatialPoint &fourth)
{
    const double firstThird = cross(first, second, third);
    const double firstFourth = cross(first, second, fourth);
    const double thirdFirst = cross(third, fourth, first);
    const double thirdSecond = cross(third, fourth, second);
    return (firstThird * firstFourth < 0.0) && (thirdFirst * thirdSecond < 0.0);
}

bool isFinite(const SpatialPoint &point)
{
    return std::isfinite(point.x) && std::isfinite(point.y);
}

bool isConvexAndOrdered(const std::array<SpatialPoint, 4> &points)
{
    double direction = 0.0;
    for (std::size_t index = 0; index < points.size(); ++index) {
        const double value = cross(points[index], points[(index + 1) % points.size()], points[(index + 2) % points.size()]);
        if (std::abs(value) < 1.0e-9) return false;
        if (direction == 0.0) direction = value;
        else if ((direction > 0.0) != (value > 0.0)) return false;
    }
    return true;
}

bool hasDuplicatePoints(const std::array<SpatialPoint, 4> &points)
{
    for (std::size_t first = 0; first < points.size(); ++first) {
        for (std::size_t second = first + 1; second < points.size(); ++second) {
            if (std::hypot(points[first].x - points[second].x, points[first].y - points[second].y) < 1.0e-6) return true;
        }
    }
    return false;
}

bool isTopLeftTopRightBottomRightBottomLeft(const std::array<SpatialPoint, 4> &points)
{
    const auto &[topLeft, topRight, bottomRight, bottomLeft] = points;
    return topLeft.x < topRight.x && bottomLeft.x < bottomRight.x && topLeft.y < bottomLeft.y && topRight.y < bottomRight.y;
}

std::array<double, 9> matrixValues(const cv::Mat &matrix)
{
    std::array<double, 9> values {};
    for (int row = 0; row < 3; ++row) {
        for (int column = 0; column < 3; ++column) values[static_cast<std::size_t>(row * 3 + column)] = matrix.at<double>(row, column);
    }
    return values;
}

} // namespace

SpatialResult<CalibrationResult> CalibrationService::calibrate(const std::array<SpatialPoint, 4> &corners,
                                                               Resolution sourceResolution) const
{
    if (sourceResolution.width <= 0 || sourceResolution.height <= 0) {
        return {false, {}, 3208, QStringLiteral("Calibration resolution is invalid")};
    }
    for (const auto &corner : corners) {
        if (corner.coordinateSystem != CoordinateSystem::ImagePixel || !isFinite(corner)
            || corner.x < 0.0 || corner.y < 0.0 || corner.x >= sourceResolution.width || corner.y >= sourceResolution.height) {
            return {false, {}, 3202, QStringLiteral("Calibration points must be IMAGE_PIXEL")};
        }
    }
    if (hasDuplicatePoints(corners) || !isTopLeftTopRightBottomRightBottomLeft(corners) || !isConvexAndOrdered(corners)
        || intersects(corners[0], corners[1], corners[2], corners[3]) || intersects(corners[1], corners[2], corners[3], corners[0])) {
        return {false, {}, 3203, QStringLiteral("Calibration quadrilateral self-intersects")};
    }
    if (polygonArea(corners) < 10000.0) return {false, {}, 3204, QStringLiteral("Calibration area is too small")};

    const std::vector<cv::Point2f> source {{static_cast<float>(corners[0].x), static_cast<float>(corners[0].y)},
                                           {static_cast<float>(corners[1].x), static_cast<float>(corners[1].y)},
                                           {static_cast<float>(corners[2].x), static_cast<float>(corners[2].y)},
                                           {static_cast<float>(corners[3].x), static_cast<float>(corners[3].y)}};
    const std::vector<cv::Point2f> destination {{0.0F, 0.0F}, {1.0F, 0.0F}, {1.0F, 1.0F}, {0.0F, 1.0F}};
    cv::Mat homography = cv::getPerspectiveTransform(source, destination);
    cv::Mat inverse;
    if (homography.empty() || !cv::checkRange(homography) || std::abs(cv::invert(homography, inverse)) < 1.0e-12 || !cv::checkRange(inverse)) {
        return {false, {}, 3206, QStringLiteral("Homography matrix is not invertible")};
    }
    const double condition = cv::norm(homography, cv::NORM_INF) * cv::norm(inverse, cv::NORM_INF);
    if (!std::isfinite(condition) || condition > 1.0e8) return {false, {}, 3206, QStringLiteral("Homography matrix is ill-conditioned")};

    std::vector<cv::Point2f> mapped;
    cv::perspectiveTransform(source, mapped, homography);
    double error = 0.0;
    for (std::size_t index = 0; index < mapped.size(); ++index) error += cv::norm(mapped[index] - destination[index]);
    error /= static_cast<double>(mapped.size());
    if (error > 3.0) return {false, {}, 3207, QStringLiteral("Reprojection error exceeds the configured limit")};

    ProjectionTarget target;
    target.targetId = astra::common::newUuid();
    target.surfaceId = astra::common::newUuid();
    target.sourceResolution = sourceResolution;
    target.targetResolution = sourceResolution;
    target.corners = corners;
    target.coordinateSystem = CoordinateSystem::ImagePixel;
    target.targetCoordinateSystem = CoordinateSystem::SurfaceLocal;
    target.homography = matrixValues(homography);
    target.inverseHomography = matrixValues(inverse);
    target.quality = SpatialQuality::Good;
    target.state = ProjectionTargetState::Calibrated;
    target.selected = true;
    target.createdAt = astra::common::utcTimestamp();
    target.updatedAt = target.createdAt;
    return {true, {target, error}, 0, {}};
}

SpatialResult<SpatialPoint> CalibrationService::map(const SpatialPoint &point, const ProjectionTarget &target) const
{
    if (point.coordinateSystem != target.coordinateSystem) {
        return {false, {}, 3403, QStringLiteral("Coordinate system is unsupported")};
    }
    const auto &matrix = target.homography;
    const double denominator = matrix[6] * point.x + matrix[7] * point.y + matrix[8];
    if (std::abs(denominator) < 1.0e-12) return {false, {}, 3404, QStringLiteral("Coordinate conversion failed")};
    return {true,
            {(matrix[0] * point.x + matrix[1] * point.y + matrix[2]) / denominator,
             (matrix[3] * point.x + matrix[4] * point.y + matrix[5]) / denominator,
             target.targetCoordinateSystem},
            0,
            {}};
}

} // namespace astra::spatial
