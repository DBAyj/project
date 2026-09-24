#include "input/FrameSource.h"
#include "vision/SurfaceDetector.h"

#include <cassert>

int main()
{
    const QString root = QStringLiteral(ASTRA_SOURCE_ROOT);
    astra::spatial::FrameSourceController source(root);
    astra::spatial::SurfaceDetector detector;

    assert(source.start(astra::spatial::FrameSourceType::Simulation,
                        root + QStringLiteral("/assets/spatial-fixtures/desk-front.png")).ok);
    const auto desk = detector.detect(source.nextFrame().value);
    assert(desk.ok);
    assert(!desk.value.isEmpty());
    const auto &candidate = desk.value.first();
    assert(candidate.confidence >= 0.5);
    assert(candidate.corners[0].coordinateSystem == astra::spatial::CoordinateSystem::ImagePixel);
    assert(candidate.areaRatio >= 0.1);

    const auto target = detector.select(desk.value, {1280, 720}, false);
    assert(target.ok);
    assert(target.value.selected);
    assert(target.value.state == astra::spatial::ProjectionTargetState::Selected);

    assert(source.start(astra::spatial::FrameSourceType::Simulation,
                        root + QStringLiteral("/assets/spatial-fixtures/no-surface.png")).ok);
    const auto empty = detector.detect(source.nextFrame().value);
    assert(empty.ok);
    assert(empty.value.isEmpty());
    assert(!detector.select(empty.value, {1280, 720}, false).ok);
}
