#include "astra/render/MeshWarp.h"

#include "astra/common/ErrorCode.h"

#include <algorithm>
#include <cmath>

namespace astra::render {
namespace {

constexpr double kEpsilon = 1e-9;
constexpr int kMaximumMeshDimension = 16;

double cross(const Point &a, const Point &b, const Point &c)
{
    return (b.x - a.x) * (c.y - a.y) - (b.y - a.y) * (c.x - a.x);
}

MeshWarpResult invalid()
{
    return {false, static_cast<int>(astra::common::ErrorCode::ProjectionMeshWarpInvalid)};
}

MeshWarpImageResult invalidImage()
{
    return {false, static_cast<int>(astra::common::ErrorCode::ProjectionMeshWarpInvalid), {}};
}

bool isInsideTriangle(const Point &point, const Point &first, const Point &second, const Point &third, Point *sourcePoint,
                      const Point &sourceFirst, const Point &sourceSecond, const Point &sourceThird)
{
    const double denominator = (second.y - third.y) * (first.x - third.x) + (third.x - second.x) * (first.y - third.y);
    if (std::abs(denominator) <= kEpsilon) return false;
    const double firstWeight = ((second.y - third.y) * (point.x - third.x) + (third.x - second.x) * (point.y - third.y)) / denominator;
    const double secondWeight = ((third.y - first.y) * (point.x - third.x) + (first.x - third.x) * (point.y - third.y)) / denominator;
    const double thirdWeight = 1.0 - firstWeight - secondWeight;
    if (firstWeight < -kEpsilon || secondWeight < -kEpsilon || thirdWeight < -kEpsilon) return false;
    *sourcePoint = {
        firstWeight * sourceFirst.x + secondWeight * sourceSecond.x + thirdWeight * sourceThird.x,
        firstWeight * sourceFirst.y + secondWeight * sourceSecond.y + thirdWeight * sourceThird.y,
    };
    return true;
}

} // namespace

MeshWarpResult MeshWarp::validate(int rows, int columns, const QVector<Point> &nodes)
{
    if (rows < 2 || columns < 2 || rows > kMaximumMeshDimension || columns > kMaximumMeshDimension || nodes.size() != rows * columns) return invalid();
    for (const Point &node : nodes) {
        if (!std::isfinite(node.x) || !std::isfinite(node.y) || node.x < 0.0 || node.x > 1.0 || node.y < 0.0 || node.y > 1.0) return invalid();
    }
    double orientation = 0.0;
    for (int row = 0; row < rows - 1; ++row) {
        for (int column = 0; column < columns - 1; ++column) {
            const auto &topLeft = nodes.at(row * columns + column);
            const auto &topRight = nodes.at(row * columns + column + 1);
            const auto &bottomRight = nodes.at((row + 1) * columns + column + 1);
            const auto &bottomLeft = nodes.at((row + 1) * columns + column);
            const double first = cross(topLeft, topRight, bottomRight);
            const double second = cross(topRight, bottomRight, bottomLeft);
            const double third = cross(bottomRight, bottomLeft, topLeft);
            const double fourth = cross(bottomLeft, topLeft, topRight);
            if (!std::isfinite(first) || !std::isfinite(second) || !std::isfinite(third) || !std::isfinite(fourth)
                || std::abs(first) <= kEpsilon || std::abs(second) <= kEpsilon || std::abs(third) <= kEpsilon || std::abs(fourth) <= kEpsilon) return invalid();
            if ((first > 0.0) != (second > 0.0) || (first > 0.0) != (third > 0.0) || (first > 0.0) != (fourth > 0.0)) return invalid();
            if (orientation == 0.0) orientation = first;
            else if ((orientation > 0.0) != (first > 0.0)) return invalid();
        }
    }
    return {true, 0};
}

MeshWarpImageResult MeshWarp::apply(const QImage &source, const MeshWarpSettings &settings, const QSize &destinationSize)
{
    if (source.isNull() || destinationSize.isEmpty() || !validate(settings.rows, settings.columns, settings.nodes).ok) return invalidImage();

    const QImage input = source.convertToFormat(QImage::Format_RGBA8888);
    QImage destination {destinationSize, QImage::Format_RGBA8888};
    if (destination.isNull()) return invalidImage();
    destination.fill(Qt::black);

    for (int y = 0; y < destination.height(); ++y) {
        for (int x = 0; x < destination.width(); ++x) {
            const Point destinationPoint {
                destination.width() == 1 ? 0.0 : static_cast<double>(x) / static_cast<double>(destination.width() - 1),
                destination.height() == 1 ? 0.0 : static_cast<double>(y) / static_cast<double>(destination.height() - 1),
            };
            bool mapped = false;
            Point sourcePoint;
            for (int row = 0; row < settings.rows - 1 && !mapped; ++row) {
                for (int column = 0; column < settings.columns - 1 && !mapped; ++column) {
                    const auto &topLeft = settings.nodes.at(row * settings.columns + column);
                    const auto &topRight = settings.nodes.at(row * settings.columns + column + 1);
                    const auto &bottomRight = settings.nodes.at((row + 1) * settings.columns + column + 1);
                    const auto &bottomLeft = settings.nodes.at((row + 1) * settings.columns + column);
                    const Point sourceTopLeft {static_cast<double>(column) / static_cast<double>(settings.columns - 1), static_cast<double>(row) / static_cast<double>(settings.rows - 1)};
                    const Point sourceTopRight {static_cast<double>(column + 1) / static_cast<double>(settings.columns - 1), static_cast<double>(row) / static_cast<double>(settings.rows - 1)};
                    const Point sourceBottomRight {static_cast<double>(column + 1) / static_cast<double>(settings.columns - 1), static_cast<double>(row + 1) / static_cast<double>(settings.rows - 1)};
                    const Point sourceBottomLeft {static_cast<double>(column) / static_cast<double>(settings.columns - 1), static_cast<double>(row + 1) / static_cast<double>(settings.rows - 1)};
                    mapped = isInsideTriangle(destinationPoint, topLeft, topRight, bottomRight, &sourcePoint,
                                              sourceTopLeft, sourceTopRight, sourceBottomRight)
                        || isInsideTriangle(destinationPoint, topLeft, bottomRight, bottomLeft, &sourcePoint,
                                            sourceTopLeft, sourceBottomRight, sourceBottomLeft);
                }
            }
            if (!mapped) continue;
            const int sourceX = std::clamp(static_cast<int>(std::lround(sourcePoint.x * static_cast<double>(input.width() - 1))), 0, input.width() - 1);
            const int sourceY = std::clamp(static_cast<int>(std::lround(sourcePoint.y * static_cast<double>(input.height() - 1))), 0, input.height() - 1);
            destination.setPixelColor(x, y, input.pixelColor(sourceX, sourceY));
        }
    }
    return {true, 0, destination};
}

} // namespace astra::render
