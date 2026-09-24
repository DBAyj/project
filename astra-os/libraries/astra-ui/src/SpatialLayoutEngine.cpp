#include "astra/ui/SpatialLayoutEngine.h"

#include <algorithm>
#include <cmath>
#include <numbers>

namespace astra::ui {
namespace {

SpatialBounds contentArea(const LayoutRequest &request)
{
    return {request.safeArea.x + request.margin, request.safeArea.y + request.margin,
            request.safeArea.width - (2.0 * request.margin), request.safeArea.height - (2.0 * request.margin)};
}

SpatialBounds clampBounds(SpatialBounds bounds, const SpatialBounds &area)
{
    bounds.width = std::min(bounds.width, area.width);
    bounds.height = std::min(bounds.height, area.height);
    bounds.x = std::clamp(bounds.x, area.x, area.x + area.width - bounds.width);
    bounds.y = std::clamp(bounds.y, area.y, area.y + area.height - bounds.height);
    return bounds;
}

} // namespace

LayoutResult SpatialLayoutEngine::apply(const LayoutRequest &request) const
{
    if (!request.safeArea.isValid() || request.margin < 0.0 || request.spacing < 0.0) {
        return {{false, 5302, QStringLiteral("Invalid layout constraints")}, {}};
    }
    const SpatialBounds area = contentArea(request);
    if (!area.isValid()) return {{false, 5304, QStringLiteral("Insufficient layout area")}, {}};
    LayoutResult result {{true, 0, {}}, {}};
    if (request.items.isEmpty()) return result;

    if (request.mode == LayoutMode::Stack) {
        double cursorX = area.x;
        double cursorY = area.y;
        for (const auto &item : request.items) {
            if (!item.bounds.isValid()) return {{false, 5302, QStringLiteral("Invalid item bounds")}, {}};
            SpatialBounds bounds {cursorX, cursorY, item.bounds.width, item.bounds.height};
            bounds = clampBounds(bounds, area);
            result.placements.append({item.componentId, bounds});
            if (request.direction == LayoutDirection::Vertical) cursorY += bounds.height + request.spacing;
            else cursorX += bounds.width + request.spacing;
        }
    } else if (request.mode == LayoutMode::Grid) {
        const int columns = request.columns > 0 ? request.columns : std::max(1, static_cast<int>(std::ceil(std::sqrt(request.items.size()))));
        const double cellWidth = (area.width - request.spacing * (columns - 1)) / columns;
        if (cellWidth <= 0.0) return {{false, 5304, QStringLiteral("Insufficient grid area")}, {}};
        double cellHeight = 0.0;
        for (const auto &item : request.items) cellHeight = std::max(cellHeight, item.bounds.height);
        for (qsizetype index = 0; index < request.items.size(); ++index) {
            const int row = static_cast<int>(index) / columns;
            const int column = static_cast<int>(index) % columns;
            SpatialBounds bounds {area.x + column * (cellWidth + request.spacing), area.y + row * (cellHeight + request.spacing), cellWidth, cellHeight};
            result.placements.append({request.items.at(index).componentId, clampBounds(bounds, area)});
        }
    } else if (request.mode == LayoutMode::Radial) {
        const double centerX = area.x + area.width / 2.0;
        const double centerY = area.y + area.height / 2.0;
        for (qsizetype index = 0; index < request.items.size(); ++index) {
            const auto &item = request.items.at(index);
            const double angle = (2.0 * std::numbers::pi * index) / request.items.size();
            SpatialBounds bounds {centerX + std::cos(angle) * request.radialRadius - item.bounds.width / 2.0,
                                  centerY + std::sin(angle) * request.radialRadius - item.bounds.height / 2.0,
                                  item.bounds.width, item.bounds.height};
            result.placements.append({item.componentId, clampBounds(bounds, area)});
        }
    } else if (request.mode == LayoutMode::Freeform) {
        for (const auto &item : request.items) result.placements.append({item.componentId, clampBounds(item.bounds, area)});
    } else if (request.mode == LayoutMode::AnchorRelative) {
        if (!request.anchor.isValid()) return {{false, 5302, QStringLiteral("Anchor is invalid")}, {}};
        for (const auto &item : request.items) {
            SpatialBounds bounds {request.anchor.x + item.bounds.x, request.anchor.y + item.bounds.y, item.bounds.width, item.bounds.height};
            result.placements.append({item.componentId, clampBounds(bounds, area)});
        }
    }
    return result;
}

} // namespace astra::ui
