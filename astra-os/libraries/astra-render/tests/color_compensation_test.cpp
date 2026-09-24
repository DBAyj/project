#include "astra/render/ColorCompensation.h"

#include <QColor>
#include <QImage>

#include <cassert>

int main()
{
    QImage frame {1, 1, QImage::Format_RGBA8888};
    frame.setPixelColor(0, 0, QColor {64, 64, 64});
    const astra::render::ColorCompensationSettings redGain {1.0, 2.0, 1.0, 1.0};
    assert(astra::render::ColorCompensationPass::apply(frame, redGain).ok);
    const QColor expectedRedGain {128, 64, 64};
    assert(frame.pixelColor(0, 0) == expectedRedGain);

    frame.setPixelColor(0, 0, QColor {128, 128, 128});
    const astra::render::ColorCompensationSettings gamma {2.0, 1.0, 1.0, 1.0};
    assert(astra::render::ColorCompensationPass::apply(frame, gamma).ok);
    assert(frame.pixelColor(0, 0).red() > 128);
    const astra::render::ColorCompensationSettings invalid {0.0, 1.0, 1.0, 1.0};
    assert(!astra::render::ColorCompensationPass::apply(frame, invalid).ok);
}
