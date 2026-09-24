#include "astra/ui/SpatialLayoutEngine.h"

#include <QtGlobal>

#include <cmath>

using namespace astra::ui;

namespace {
void require(bool condition)
{
    if (!condition) qFatal("P5 spatial layout requirement failed");
}
}

int main()
{
    SpatialLayoutEngine engine;
    LayoutRequest request;
    request.safeArea = {0.0, 0.0, 600.0, 400.0};
    request.margin = 20.0;
    request.spacing = 10.0;
    request.items = {{QStringLiteral("one"), {50.0, 40.0, 100.0, 50.0}}, {QStringLiteral("two"), {500.0, 360.0, 100.0, 50.0}}};

    request.mode = LayoutMode::Stack;
    request.direction = LayoutDirection::Vertical;
    auto result = engine.apply(request);
    require(result.ok && result.placements.size() == 2);
    require(result.placements.at(0).bounds.x == 20.0 && result.placements.at(0).bounds.y == 20.0);
    require(result.placements.at(1).bounds.y == 80.0);

    request.mode = LayoutMode::Grid;
    request.columns = 2;
    result = engine.apply(request);
    require(result.ok);
    require(result.placements.at(0).bounds.width == 275.0);
    require(result.placements.at(1).bounds.x == 305.0);

    request.mode = LayoutMode::Radial;
    request.radialRadius = 100.0;
    result = engine.apply(request);
    require(result.ok);
    require(std::abs(result.placements.at(0).bounds.x - 350.0) < 0.001);

    request.mode = LayoutMode::Freeform;
    result = engine.apply(request);
    require(result.ok);
    require(result.placements.at(1).bounds.x == 480.0);
    require(result.placements.at(1).bounds.y == 330.0);

    request.mode = LayoutMode::AnchorRelative;
    request.anchor = {250.0, 180.0, 20.0, 20.0};
    result = engine.apply(request);
    require(result.ok);
    require(result.placements.at(0).bounds.x == 300.0);
    require(result.placements.at(0).bounds.y == 220.0);

    request.safeArea = {0.0, 0.0, 20.0, 20.0};
    request.margin = 20.0;
    const auto insufficient = engine.apply(request);
    require(!insufficient.ok && insufficient.errorCode == 5304);
    require(insufficient.placements.isEmpty());
}
