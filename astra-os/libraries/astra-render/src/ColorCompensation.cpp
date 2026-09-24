#include "astra/render/ColorCompensation.h"

#include <QColor>

#include <algorithm>
#include <cmath>

namespace astra::render {
namespace {

int compensate(int value, double gain, double gamma)
{
    const double normalized = static_cast<double>(value) / 255.0;
    const double corrected = std::pow(normalized, 1.0 / gamma) * gain;
    return static_cast<int>(std::lround(std::clamp(corrected, 0.0, 1.0) * 255.0));
}

} // namespace

ColorCompensationResult ColorCompensationPass::apply(QImage &frame, const ColorCompensationSettings &settings)
{
    if (frame.isNull() || settings.gamma <= 0.0 || settings.gamma > 4.0
        || settings.redGain <= 0.0 || settings.redGain > 4.0
        || settings.greenGain <= 0.0 || settings.greenGain > 4.0
        || settings.blueGain <= 0.0 || settings.blueGain > 4.0) return {false};
    for (int y = 0; y < frame.height(); ++y) {
        for (int x = 0; x < frame.width(); ++x) {
            const QColor color = frame.pixelColor(x, y);
            frame.setPixelColor(x, y, QColor {compensate(color.red(), settings.redGain, settings.gamma), compensate(color.green(), settings.greenGain, settings.gamma), compensate(color.blue(), settings.blueGain, settings.gamma), color.alpha()});
        }
    }
    return {true};
}

} // namespace astra::render
