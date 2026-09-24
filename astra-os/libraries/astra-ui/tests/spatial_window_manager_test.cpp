#include "astra/ui/SpatialWindowManager.h"

#include <QtGlobal>

using namespace astra::ui;

namespace {
void require(bool condition)
{
    if (!condition) qFatal("P5 spatial window requirement failed");
}
}

int main()
{
    SpatialWindowManager windows {1, {0.0, 0.0, 1280.0, 720.0}};
    SpatialWindowSpec spec;
    spec.windowId = QStringLiteral("window-1");
    spec.componentId = QStringLiteral("component-1");
    spec.bounds = {100.0, 80.0, 400.0, 240.0};
    spec.displayTarget = DisplayTarget::Both;
    spec.projectionTarget = QStringLiteral("fixture-front");
    spec.focusScope = QStringLiteral("workspace");
    spec.privacyLevel = astra::common::PrivacyLevel::Public;

    require(windows.createWindow(spec).ok);
    require(windows.showWindow(spec.windowId).ok);
    require(windows.find(spec.windowId)->state() == SpatialWindowState::Visible);
    require(windows.moveWindow(spec.windowId, 1200.0, 700.0).ok);
    const auto clamped = windows.find(spec.windowId)->bounds();
    require(clamped.x == 880.0);
    require(clamped.y == 480.0);

    const auto beforeInvalidResize = windows.find(spec.windowId)->bounds();
    const auto invalidResize = windows.resizeWindow(spec.windowId, 0.0, 50.0);
    require(!invalidResize.ok);
    require(invalidResize.errorCode == 5205);
    require(windows.find(spec.windowId)->bounds().width == beforeInvalidResize.width);

    require(windows.resizeWindow(spec.windowId, 2000.0, 1000.0).ok);
    const auto resized = windows.find(spec.windowId)->bounds();
    require(resized.width == 1280.0);
    require(resized.height == 720.0);
    require(resized.x == 0.0 && resized.y == 0.0);

    SpatialWindowSpec second = spec;
    second.windowId = QStringLiteral("window-2");
    require(windows.createWindow(second).errorCode == 5202);
    require(windows.hideWindow(spec.windowId).ok);
    require(windows.find(spec.windowId)->state() == SpatialWindowState::Hidden);
    require(windows.closeWindow(spec.windowId).ok);
    require(windows.find(spec.windowId)->state() == SpatialWindowState::Closed);
    require(windows.findByComponentId(spec.componentId) == nullptr);

    require(windows.createWindow(second).ok);
    require(windows.createWindow(spec).errorCode == 5202);
    require(windows.removeWindow(second.windowId).ok);
    require(windows.find(second.windowId) == nullptr);
    require(windows.removeWindow(second.windowId).errorCode == 5201);
    require(windows.createWindow(spec).ok);
    require(windows.find(spec.windowId)->state() == SpatialWindowState::Created);
    require(windows.findByComponentId(spec.componentId) == windows.find(spec.windowId));
}
