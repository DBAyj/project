#include "astra/render/PrivacyMask.h"

#include "astra/common/ErrorCode.h"

#include <QPainter>

namespace astra::render {
namespace {

PrivacyMaskResult invalid()
{
    return {false, static_cast<int>(astra::common::ErrorCode::ProjectionPrivacyMaskInvalid)};
}

} // namespace

PrivacyMaskResult PrivacyMaskPass::apply(QImage &frame, const PrivacyMask &mask)
{
    if (frame.isNull() || !mask.color.isValid()) return invalid();
    if (mask.mode == MaskMode::FullClear || mask.mode == MaskMode::SolidMask) {
        frame.fill(mask.color);
        return {true, 0};
    }
    if (mask.mode == MaskMode::RectangleMask) {
        if (!mask.rectangle.isValid() || !frame.rect().intersects(mask.rectangle)) return invalid();
        QPainter painter {&frame};
        painter.fillRect(mask.rectangle, mask.color);
        return {true, 0};
    }
    if (mask.mode == MaskMode::PolygonMask) {
        if (mask.polygon.size() < 3 || !frame.rect().intersects(mask.polygon.boundingRect())) return invalid();
        QPainter painter {&frame};
        painter.setPen(Qt::NoPen);
        painter.setBrush(mask.color);
        painter.drawPolygon(mask.polygon);
        return {true, 0};
    }
    return invalid();
}

} // namespace astra::render
