#pragma once

#include "astra/ui/Types.h"

#include <QList>

namespace astra::ui {

enum class LayoutMode { Stack, Grid, Radial, Freeform, AnchorRelative };
enum class LayoutDirection { Vertical, Horizontal };

struct LayoutItem {
    QString componentId;
    SpatialBounds bounds;
};

struct LayoutRequest {
    LayoutMode mode {LayoutMode::Stack};
    LayoutDirection direction {LayoutDirection::Vertical};
    QList<LayoutItem> items;
    SpatialBounds safeArea;
    SpatialBounds anchor;
    double spacing {16.0};
    double margin {24.0};
    int columns {0};
    double radialRadius {120.0};
    bool keepInsideTarget {true};
};

struct LayoutPlacement {
    QString componentId;
    SpatialBounds bounds;
};

struct LayoutResult : OperationResult {
    QList<LayoutPlacement> placements;
};

class SpatialLayoutEngine final {
public:
    [[nodiscard]] LayoutResult apply(const LayoutRequest &request) const;
};

} // namespace astra::ui
