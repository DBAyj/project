#pragma once

#include "astra/render/Homography.h"

#include <QImage>
#include <QVector>

namespace astra::render {

struct MeshWarpResult {
    bool ok {false};
    int errorCode {0};
};

struct MeshWarpSettings {
    int rows {0};
    int columns {0};
    QVector<Point> nodes;
};

struct MeshWarpImageResult {
    bool ok {false};
    int errorCode {0};
    QImage frame;
};

class MeshWarp final {
public:
    static MeshWarpResult validate(int rows, int columns, const QVector<Point> &nodes);
    static MeshWarpImageResult apply(const QImage &source, const MeshWarpSettings &settings, const QSize &destinationSize);
};

} // namespace astra::render
