#include "astra/render/Homography.h"

#include <QColor>
#include <QImage>

#include <cassert>
#include <cmath>
#include <limits>

int main()
{
    using astra::render::Homography;
    using astra::render::Point;
    using astra::render::Resolution;

    const Resolution output {1280, 720, 60.0};
    const auto front = Homography::fromUnitSquare({Point {0.0, 0.0}, Point {1279.0, 0.0}, Point {1279.0, 719.0}, Point {0.0, 719.0}}, output);
    assert(front.ok);
    const auto mappedCenter = Homography::map(front.matrix, Point {0.5, 0.5});
    assert(std::abs(mappedCenter.x - 639.5) < 0.001);
    assert(std::abs(mappedCenter.y - 359.5) < 0.001);

    for (const auto &corners : {
             std::array<Point, 4> {Point {140.0, 0.0}, Point {1139.0, 0.0}, Point {1279.0, 719.0}, Point {0.0, 719.0}},
             std::array<Point, 4> {Point {0.0, 0.0}, Point {1139.0, 0.0}, Point {1279.0, 719.0}, Point {140.0, 719.0}},
             std::array<Point, 4> {Point {0.0, 120.0}, Point {1279.0, 120.0}, Point {1279.0, 719.0}, Point {0.0, 719.0}},
             std::array<Point, 4> {Point {0.0, 0.0}, Point {1279.0, 0.0}, Point {1139.0, 599.0}, Point {140.0, 599.0}},
         }) {
        assert(Homography::fromUnitSquare(corners, output).ok);
    }

    const auto selfIntersecting = Homography::fromUnitSquare({Point {0.0, 0.0}, Point {1279.0, 719.0}, Point {0.0, 719.0}, Point {1279.0, 0.0}}, output);
    assert(!selfIntersecting.ok);
    assert(selfIntersecting.errorCode == 4011);
    const auto outOfBounds = Homography::fromUnitSquare({Point {-400.0, 0.0}, Point {1279.0, 0.0}, Point {1279.0, 719.0}, Point {0.0, 719.0}}, output);
    assert(!outOfBounds.ok);
    assert(outOfBounds.errorCode == 4013);
    const auto nonFinite = Homography::fromUnitSquare({Point {0.0, 0.0}, Point {std::numeric_limits<double>::infinity(), 0.0}, Point {1279.0, 719.0}, Point {0.0, 719.0}}, output);
    assert(!nonFinite.ok);
    assert(nonFinite.errorCode == 4011);
    const astra::render::Matrix3 singular {1.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 1.0};
    assert(!Homography::validateMatrix(singular).ok);
    assert(Homography::validateMatrix(singular).errorCode == 4012);

    QImage source {3, 3, QImage::Format_RGBA8888};
    source.fill(Qt::black);
    source.setPixelColor(1, 1, Qt::red);
    const auto warped = Homography::warp(source, front.inverse, QSize {1280, 720});
    assert(!warped.isNull());
    assert(warped.pixelColor(640, 360) == QColor(Qt::red));
}
