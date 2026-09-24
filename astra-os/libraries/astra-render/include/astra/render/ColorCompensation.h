#pragma once

#include <QImage>

namespace astra::render {

struct ColorCompensationSettings {
    double gamma {1.0};
    double redGain {1.0};
    double greenGain {1.0};
    double blueGain {1.0};
};

struct ColorCompensationResult { bool ok {false}; };

class ColorCompensationPass final {
public:
    static ColorCompensationResult apply(QImage &frame, const ColorCompensationSettings &settings);
};

} // namespace astra::render
