#pragma once

#include <QImage>
#include <QRectF>

namespace astra::render {

struct CropOverscanSettings {
    QRectF normalizedCrop {0.0, 0.0, 1.0, 1.0};
    double overscan {0.0};
};

struct CropOverscanResult {
    bool ok {false};
    QImage frame;
};

class CropOverscanPass final {
public:
    static CropOverscanResult apply(const QImage &source, const CropOverscanSettings &settings);
};

} // namespace astra::render
