#pragma once

#include "astra/render/ProjectionOutput.h"

#include <QImage>

#include <array>

namespace astra::render {

struct Point {
    double x {0.0};
    double y {0.0};
};

using Matrix3 = std::array<double, 9>;

struct HomographyResult {
    bool ok {false};
    int errorCode {0};
    Matrix3 matrix {};
    Matrix3 inverse {};
};

class Homography final {
public:
    static HomographyResult fromUnitSquare(const std::array<Point, 4> &corners, const Resolution &bounds);
    static HomographyResult validateMatrix(const Matrix3 &matrix);
    static Point map(const Matrix3 &matrix, Point source);
    static QImage warp(const QImage &source, const Matrix3 &inverse, const QSize &destinationSize);
};

} // namespace astra::render
