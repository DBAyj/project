#include "astra/render/CropOverscan.h"

#include <algorithm>
#include <cmath>

namespace astra::render {

CropOverscanResult CropOverscanPass::apply(const QImage &source, const CropOverscanSettings &settings)
{
    if (source.isNull() || settings.overscan < 0.0 || settings.overscan > 0.25) return {false, {}};
    const QRectF crop = settings.normalizedCrop;
    if (crop.x() < 0.0 || crop.y() < 0.0 || crop.width() <= 0.0 || crop.height() <= 0.0
        || crop.right() > 1.0 || crop.bottom() > 1.0) return {false, {}};
    const double expandedX = std::max(0.0, crop.x() - settings.overscan);
    const double expandedY = std::max(0.0, crop.y() - settings.overscan);
    const double expandedRight = std::min(1.0, crop.right() + settings.overscan);
    const double expandedBottom = std::min(1.0, crop.bottom() + settings.overscan);
    const QRect pixelCrop {
        static_cast<int>(std::floor(expandedX * source.width())),
        static_cast<int>(std::floor(expandedY * source.height())),
        static_cast<int>(std::ceil((expandedRight - expandedX) * source.width())),
        static_cast<int>(std::ceil((expandedBottom - expandedY) * source.height())),
    };
    if (!source.rect().contains(pixelCrop) || pixelCrop.isEmpty()) return {false, {}};
    return {true, source.copy(pixelCrop).scaled(source.size(), Qt::IgnoreAspectRatio, Qt::SmoothTransformation)};
}

} // namespace astra::render
