#include "astra/render/Homography.h"

#include "astra/common/ErrorCode.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <limits>

namespace astra::render {
namespace {

constexpr double kEpsilon = 1e-9;

HomographyResult invalid()
{
    return {false, static_cast<int>(astra::common::ErrorCode::ProjectionHomographyInvalid), {}, {}};
}

HomographyResult nonInvertible()
{
    return {false, static_cast<int>(astra::common::ErrorCode::ProjectionHomographyNonInvertible), {}, {}};
}

double cross(const Point &a, const Point &b, const Point &c)
{
    return (b.x - a.x) * (c.y - a.y) - (b.y - a.y) * (c.x - a.x);
}

bool pointOnSegment(const Point &a, const Point &b, const Point &point)
{
    return std::abs(cross(a, b, point)) <= kEpsilon
        && point.x >= std::min(a.x, b.x) - kEpsilon && point.x <= std::max(a.x, b.x) + kEpsilon
        && point.y >= std::min(a.y, b.y) - kEpsilon && point.y <= std::max(a.y, b.y) + kEpsilon;
}

bool segmentsIntersect(const Point &a, const Point &b, const Point &c, const Point &d)
{
    const double abC = cross(a, b, c);
    const double abD = cross(a, b, d);
    const double cdA = cross(c, d, a);
    const double cdB = cross(c, d, b);
    if (((abC > kEpsilon && abD < -kEpsilon) || (abC < -kEpsilon && abD > kEpsilon))
        && ((cdA > kEpsilon && cdB < -kEpsilon) || (cdA < -kEpsilon && cdB > kEpsilon))) return true;
    return pointOnSegment(a, b, c) || pointOnSegment(a, b, d) || pointOnSegment(c, d, a) || pointOnSegment(c, d, b);
}

bool isConvex(const std::array<Point, 4> &corners)
{
    double direction = 0.0;
    for (std::size_t index = 0; index < corners.size(); ++index) {
        const double current = cross(corners[index], corners[(index + 1) % corners.size()], corners[(index + 2) % corners.size()]);
        if (std::abs(current) <= kEpsilon) return false;
        if (direction == 0.0) direction = current;
        else if ((direction > 0.0) != (current > 0.0)) return false;
    }
    return true;
}

double polygonArea(const std::array<Point, 4> &corners)
{
    double twiceArea = 0.0;
    for (std::size_t index = 0; index < corners.size(); ++index) {
        const auto &current = corners[index];
        const auto &next = corners[(index + 1) % corners.size()];
        twiceArea += current.x * next.y - next.x * current.y;
    }
    return std::abs(twiceArea) * 0.5;
}

bool solve(std::array<std::array<double, 9>, 8> &system, std::array<double, 8> &solution)
{
    for (std::size_t column = 0; column < 8; ++column) {
        std::size_t pivot = column;
        for (std::size_t row = column + 1; row < 8; ++row) {
            if (std::abs(system[row][column]) > std::abs(system[pivot][column])) pivot = row;
        }
        if (std::abs(system[pivot][column]) <= kEpsilon) return false;
        std::swap(system[pivot], system[column]);
        const double pivotValue = system[column][column];
        for (std::size_t entry = column; entry < 9; ++entry) system[column][entry] /= pivotValue;
        for (std::size_t row = 0; row < 8; ++row) {
            if (row == column) continue;
            const double factor = system[row][column];
            for (std::size_t entry = column; entry < 9; ++entry) system[row][entry] -= factor * system[column][entry];
        }
    }
    for (std::size_t row = 0; row < 8; ++row) solution[row] = system[row][8];
    return true;
}

} // namespace

HomographyResult Homography::fromUnitSquare(const std::array<Point, 4> &corners, const Resolution &bounds)
{
    if (!bounds.isValid()) return invalid();
    const double minimumX = -static_cast<double>(bounds.width) * 0.25;
    const double maximumX = static_cast<double>(bounds.width) * 1.25;
    const double minimumY = -static_cast<double>(bounds.height) * 0.25;
    const double maximumY = static_cast<double>(bounds.height) * 1.25;
    for (const auto &corner : corners) {
        if (!std::isfinite(corner.x) || !std::isfinite(corner.y)) return invalid();
        if (corner.x < minimumX || corner.x > maximumX || corner.y < minimumY || corner.y > maximumY) {
            return {false, static_cast<int>(astra::common::ErrorCode::ProjectionHomographyOutOfBounds), {}, {}};
        }
    }
    if (segmentsIntersect(corners[0], corners[1], corners[2], corners[3])
        || segmentsIntersect(corners[1], corners[2], corners[3], corners[0])
        || !isConvex(corners) || polygonArea(corners) < 16.0) return invalid();

    constexpr std::array<Point, 4> source {{{0.0, 0.0}, {1.0, 0.0}, {1.0, 1.0}, {0.0, 1.0}}};
    std::array<std::array<double, 9>, 8> system {};
    for (std::size_t index = 0; index < 4; ++index) {
        const double x = source[index].x;
        const double y = source[index].y;
        const double u = corners[index].x;
        const double v = corners[index].y;
        system[index * 2] = {x, y, 1.0, 0.0, 0.0, 0.0, -u * x, -u * y, u};
        system[index * 2 + 1] = {0.0, 0.0, 0.0, x, y, 1.0, -v * x, -v * y, v};
    }
    std::array<double, 8> solution {};
    if (!solve(system, solution)) return nonInvertible();
    const Matrix3 matrix {solution[0], solution[1], solution[2], solution[3], solution[4], solution[5], solution[6], solution[7], 1.0};
    const auto validated = validateMatrix(matrix);
    if (!validated.ok) return validated;
    return {true, 0, matrix, validated.inverse};
}

HomographyResult Homography::validateMatrix(const Matrix3 &matrix)
{
    for (const double value : matrix) {
        if (!std::isfinite(value)) return invalid();
    }
    const double determinant = matrix[0] * (matrix[4] * matrix[8] - matrix[5] * matrix[7])
        - matrix[1] * (matrix[3] * matrix[8] - matrix[5] * matrix[6])
        + matrix[2] * (matrix[3] * matrix[7] - matrix[4] * matrix[6]);
    if (!std::isfinite(determinant) || std::abs(determinant) <= kEpsilon) return nonInvertible();
    const double inverseDeterminant = 1.0 / determinant;
    const Matrix3 inverse {
        (matrix[4] * matrix[8] - matrix[5] * matrix[7]) * inverseDeterminant,
        (matrix[2] * matrix[7] - matrix[1] * matrix[8]) * inverseDeterminant,
        (matrix[1] * matrix[5] - matrix[2] * matrix[4]) * inverseDeterminant,
        (matrix[5] * matrix[6] - matrix[3] * matrix[8]) * inverseDeterminant,
        (matrix[0] * matrix[8] - matrix[2] * matrix[6]) * inverseDeterminant,
        (matrix[2] * matrix[3] - matrix[0] * matrix[5]) * inverseDeterminant,
        (matrix[3] * matrix[7] - matrix[4] * matrix[6]) * inverseDeterminant,
        (matrix[1] * matrix[6] - matrix[0] * matrix[7]) * inverseDeterminant,
        (matrix[0] * matrix[4] - matrix[1] * matrix[3]) * inverseDeterminant,
    };
    return {true, 0, matrix, inverse};
}

Point Homography::map(const Matrix3 &matrix, Point source)
{
    const double denominator = matrix[6] * source.x + matrix[7] * source.y + matrix[8];
    if (std::abs(denominator) <= kEpsilon) {
        const double invalidValue = std::numeric_limits<double>::quiet_NaN();
        return {invalidValue, invalidValue};
    }
    return {
        (matrix[0] * source.x + matrix[1] * source.y + matrix[2]) / denominator,
        (matrix[3] * source.x + matrix[4] * source.y + matrix[5]) / denominator,
    };
}

QImage Homography::warp(const QImage &source, const Matrix3 &inverse, const QSize &destinationSize)
{
    if (source.isNull() || destinationSize.isEmpty() || !validateMatrix(inverse).ok) return {};
    QImage destination {destinationSize, QImage::Format_RGBA8888};
    destination.fill(Qt::black);
    const QImage input = source.convertToFormat(QImage::Format_RGBA8888);
    for (int y = 0; y < destination.height(); ++y) {
        for (int x = 0; x < destination.width(); ++x) {
            const Point mapped = map(inverse, {static_cast<double>(x), static_cast<double>(y)});
            if (!std::isfinite(mapped.x) || !std::isfinite(mapped.y) || mapped.x < 0.0 || mapped.x > 1.0 || mapped.y < 0.0 || mapped.y > 1.0) continue;
            const int sourceX = std::clamp(static_cast<int>(std::lround(mapped.x * static_cast<double>(input.width() - 1))), 0, input.width() - 1);
            const int sourceY = std::clamp(static_cast<int>(std::lround(mapped.y * static_cast<double>(input.height() - 1))), 0, input.height() - 1);
            destination.setPixelColor(x, y, input.pixelColor(sourceX, sourceY));
        }
    }
    return destination;
}

} // namespace astra::render
