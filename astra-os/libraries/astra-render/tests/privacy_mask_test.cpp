#include "astra/render/PrivacyMask.h"

#include <QColor>
#include <QImage>

#include <cassert>

int main()
{
    using astra::render::MaskMode;
    using astra::render::PrivacyMask;
    using astra::render::PrivacyMaskPass;

    QImage image {100, 100, QImage::Format_RGBA8888};
    image.fill(Qt::red);
    const PrivacyMask rectangleMask {MaskMode::RectangleMask, QColor(Qt::black), QRect {10, 20, 30, 40}, {}};
    assert(PrivacyMaskPass::apply(image, rectangleMask).ok);
    assert(image.pixelColor(10, 20) == QColor(Qt::black));
    assert(image.pixelColor(0, 0) == QColor(Qt::red));

    image.fill(Qt::red);
    const QPolygon polygon {{QPoint {10, 10}, QPoint {90, 10}, QPoint {50, 90}}};
    const PrivacyMask polygonMask {MaskMode::PolygonMask, QColor(Qt::black), {}, polygon};
    assert(PrivacyMaskPass::apply(image, polygonMask).ok);
    assert(image.pixelColor(50, 30) == QColor(Qt::black));

    image.fill(Qt::red);
    const PrivacyMask clearMask {MaskMode::FullClear, QColor(Qt::black), {}, {}};
    assert(PrivacyMaskPass::apply(image, clearMask).ok);
    assert(image.pixelColor(0, 0) == QColor(Qt::black));
    const PrivacyMask invalidMask {MaskMode::PolygonMask, QColor(Qt::black), {}, {QPoint {0, 0}, QPoint {1, 1}}};
    const auto invalidResult = PrivacyMaskPass::apply(image, invalidMask);
    assert(!invalidResult.ok);
    assert(invalidResult.errorCode == 4015);
}
