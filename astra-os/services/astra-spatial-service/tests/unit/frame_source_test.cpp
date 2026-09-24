#include "input/FrameSource.h"

#include <cassert>

int main()
{
    const QString root = QStringLiteral(ASTRA_SOURCE_ROOT);
    astra::spatial::FrameSourceController source(root);

    const auto simulation = source.start(astra::spatial::FrameSourceType::Simulation,
                                         root + QStringLiteral("/assets/spatial-fixtures/desk-front.png"));
    assert(simulation.ok);
    assert(source.running());
    const auto frame = source.nextFrame();
    assert(frame.ok);
    assert(frame.value.frame.source == astra::spatial::FrameSourceType::Simulation);
    assert(frame.value.frame.resolution.width == 1280);
    assert(frame.value.frame.resolution.height == 720);
    assert(!frame.value.pixels.empty());

    const auto image = source.start(astra::spatial::FrameSourceType::Image,
                                    root + QStringLiteral("/assets/spatial-fixtures/wall-front.png"));
    assert(image.ok);
    assert(source.nextFrame().value.frame.source == astra::spatial::FrameSourceType::Image);

    const auto missing = source.start(astra::spatial::FrameSourceType::Image, QStringLiteral("missing.png"));
    assert(!missing.ok);
    assert(missing.errorCode == 3004);
    assert(source.start(astra::spatial::FrameSourceType::Simulation,
                        root + QStringLiteral("/assets/spatial-fixtures/no-surface.png")).ok);
    assert(source.stop().ok);
    assert(!source.running());
    assert(!source.nextFrame().ok);
}
