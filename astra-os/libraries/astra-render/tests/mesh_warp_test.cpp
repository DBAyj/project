#include "astra/render/MeshWarp.h"

#include <QColor>

#include <cassert>

int main()
{
    using astra::render::MeshWarp;
    using astra::render::Point;

    const QVector<Point> grid {{0.0, 0.0}, {1.0, 0.0}, {0.0, 1.0}, {1.0, 1.0}};
    assert(MeshWarp::validate(2, 2, grid).ok);
    const QVector<Point> folded {{0.0, 0.0}, {1.0, 0.0}, {1.0, 1.0}, {0.0, 1.0}};
    const auto invalid = MeshWarp::validate(2, 2, folded);
    assert(!invalid.ok);
    assert(invalid.errorCode == 4014);

    QImage source {5, 5, QImage::Format_RGBA8888};
    source.fill(QColor {40, 192, 128});
    const astra::render::MeshWarpSettings clipped {2, 2, {{0.0, 0.0}, {0.5, 0.0}, {0.0, 1.0}, {0.5, 1.0}}};
    const auto applied = MeshWarp::apply(source, clipped, source.size());
    assert(applied.ok);
    assert((applied.frame.pixelColor(2, 2) == QColor {40, 192, 128}));
    assert(applied.frame.pixelColor(3, 2) == QColor(Qt::black));
}
