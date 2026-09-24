#include "astra/render/CropOverscan.h"

#include <QColor>
#include <QImage>

#include <cassert>

int main()
{
    QImage source {4, 2, QImage::Format_RGBA8888};
    for (int y = 0; y < source.height(); ++y) {
        source.setPixelColor(0, y, Qt::red);
        source.setPixelColor(1, y, Qt::red);
        source.setPixelColor(2, y, Qt::blue);
        source.setPixelColor(3, y, Qt::blue);
    }

    const astra::render::CropOverscanSettings settings {QRectF {0.5, 0.0, 0.5, 1.0}, 0.0};
    const auto cropped = astra::render::CropOverscanPass::apply(source, settings);
    assert(cropped.ok);
    assert(cropped.frame.size() == source.size());
    assert(cropped.frame.pixelColor(0, 0) == QColor(Qt::blue));
    assert(cropped.frame.pixelColor(3, 1) == QColor(Qt::blue));

    const astra::render::CropOverscanSettings invalidSettings {QRectF {0.9, 0.0, 0.2, 1.0}, 0.0};
    const auto invalid = astra::render::CropOverscanPass::apply(source, invalidSettings);
    assert(!invalid.ok);
}
