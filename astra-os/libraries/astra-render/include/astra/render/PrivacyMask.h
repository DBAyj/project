#pragma once

#include <QColor>
#include <QImage>
#include <QPolygon>
#include <QRect>

namespace astra::render {

enum class MaskMode { FullClear, RectangleMask, PolygonMask, SolidMask };

struct PrivacyMask {
    MaskMode mode {MaskMode::FullClear};
    QColor color {Qt::black};
    QRect rectangle;
    QPolygon polygon;
};

struct PrivacyMaskResult {
    bool ok {false};
    int errorCode {0};
};

class PrivacyMaskPass final {
public:
    static PrivacyMaskResult apply(QImage &frame, const PrivacyMask &mask);
};

} // namespace astra::render
