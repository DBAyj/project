#include "astra/render/ProjectionOutput.h"

#include <QColor>
#include <QImage>

#include <cassert>

int main()
{
    using astra::render::OutputState;
    using astra::render::Resolution;
    using astra::render::WindowProjectionOutput;

    WindowProjectionOutput output {"window-projection"};
    const auto initialized = output.initialize(Resolution {4, 3, 60.0});
    assert(initialized.ok);
    assert(output.state() == OutputState::Ready);
    const Resolution expectedResolution {4, 3, 60.0};
    assert(output.resolution() == expectedResolution);

    QImage frame {4, 3, QImage::Format_RGBA8888};
    frame.fill(Qt::red);
    assert(output.present(frame).ok);
    assert(output.state() == OutputState::Presenting);
    assert(output.frame().pixelColor(0, 0) == QColor(Qt::red));

    assert(output.clear().ok);
    assert(output.state() == OutputState::Cleared);
    assert(output.frame().pixelColor(0, 0) == QColor(Qt::black));

    output.setConnected(false);
    assert(!output.present(frame).ok);
    assert(output.present(frame).errorCode == 4007);
    assert(output.shutdown().ok);
    assert(output.state() == OutputState::Stopped);
}
