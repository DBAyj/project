#include "astra/ui/Types.h"

#include <cmath>

namespace astra::ui {

bool SpatialBounds::isValid() const
{
    return std::isfinite(x) && std::isfinite(y) && std::isfinite(width) && std::isfinite(height)
        && width > 0.0 && height > 0.0;
}

bool SpatialBounds::contains(double pointX, double pointY) const
{
    return pointX >= x && pointY >= y && pointX <= x + width && pointY <= y + height;
}

bool isSystemOnly(ComponentType type)
{
    return type == ComponentType::SystemSecurityOverlay || type == ComponentType::PrivacyMask
        || type == ComponentType::SystemCriticalAlert;
}

} // namespace astra::ui
