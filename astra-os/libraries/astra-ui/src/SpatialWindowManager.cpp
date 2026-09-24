#include "astra/ui/SpatialWindowManager.h"

#include <algorithm>
#include <cmath>

namespace astra::ui {

SpatialWindow::SpatialWindow(SpatialWindowSpec spec)
    : spec_(std::move(spec))
{
}

QString SpatialWindow::windowId() const { return spec_.windowId; }
QString SpatialWindow::componentId() const { return spec_.componentId; }
SpatialBounds SpatialWindow::bounds() const { return spec_.bounds; }
SpatialWindowState SpatialWindow::state() const { return state_; }
DisplayTarget SpatialWindow::displayTarget() const { return spec_.displayTarget; }
QString SpatialWindow::projectionTarget() const { return spec_.projectionTarget; }
QString SpatialWindow::anchorId() const { return spec_.anchorId; }
astra::common::PrivacyLevel SpatialWindow::privacyLevel() const { return spec_.privacyLevel; }
QString SpatialWindow::focusScope() const { return spec_.focusScope; }

SpatialWindowManager::SpatialWindowManager(qsizetype maximumWindows, SpatialBounds safeArea)
    : maximumWindows_(maximumWindows)
    , safeArea_(safeArea)
{
}

OperationResult SpatialWindowManager::createWindow(const SpatialWindowSpec &spec)
{
    const auto existing = windows_.value(spec.windowId);
    if (existing && existing->state() != SpatialWindowState::Closed) return {false, 5203, QStringLiteral("Window already exists")};
    // Closed windows stay queryable but no longer hold a window slot.
    if (openWindowCount() >= maximumWindows_) return {false, 5202, QStringLiteral("Window limit exceeded")};
    if (spec.windowId.isEmpty() || spec.componentId.isEmpty() || spec.focusScope.isEmpty() || !spec.bounds.isValid()) {
        return {false, 5205, QStringLiteral("Invalid window specification")};
    }
    SpatialWindowSpec bounded = spec;
    bounded.bounds = clamp(spec.bounds);
    auto window = std::make_shared<SpatialWindow>(bounded);
    windows_.insert(spec.windowId, std::move(window));
    defaults_.insert(spec.windowId, bounded.bounds);
    return {true, 0, {}};
}

OperationResult SpatialWindowManager::showWindow(const QString &windowId)
{
    auto *window = find(windowId);
    if (!window) return {false, 5201, QStringLiteral("Window not found")};
    if (window->state_ != SpatialWindowState::Created && window->state_ != SpatialWindowState::Hidden && window->state_ != SpatialWindowState::Minimized) {
        return {false, 5203, QStringLiteral("Window cannot be shown from its current state")};
    }
    window->state_ = SpatialWindowState::Opening;
    window->state_ = SpatialWindowState::Visible;
    return {true, 0, {}};
}

OperationResult SpatialWindowManager::hideWindow(const QString &windowId)
{
    auto *window = find(windowId);
    if (!window) return {false, 5201, QStringLiteral("Window not found")};
    if (window->state_ != SpatialWindowState::Visible) return {false, 5203, QStringLiteral("Only visible windows may be hidden")};
    window->state_ = SpatialWindowState::Hidden;
    return {true, 0, {}};
}

OperationResult SpatialWindowManager::closeWindow(const QString &windowId)
{
    auto *window = find(windowId);
    if (!window) return {false, 5201, QStringLiteral("Window not found")};
    if (window->state_ != SpatialWindowState::Visible && window->state_ != SpatialWindowState::Hidden
        && window->state_ != SpatialWindowState::Minimized && window->state_ != SpatialWindowState::Created) {
        return {false, 5203, QStringLiteral("Window cannot be closed from its current state")};
    }
    window->state_ = SpatialWindowState::Closing;
    window->state_ = SpatialWindowState::Closed;
    return {true, 0, {}};
}

OperationResult SpatialWindowManager::moveWindow(const QString &windowId, double x, double y)
{
    auto *window = find(windowId);
    if (!window) return {false, 5201, QStringLiteral("Window not found")};
    if (!std::isfinite(x) || !std::isfinite(y)) return {false, 5204, QStringLiteral("Invalid window position")};
    auto bounds = window->spec_.bounds;
    bounds.x = x;
    bounds.y = y;
    window->spec_.bounds = clamp(bounds);
    return {true, 0, {}};
}

OperationResult SpatialWindowManager::resizeWindow(const QString &windowId, double width, double height)
{
    auto *window = find(windowId);
    if (!window) return {false, 5201, QStringLiteral("Window not found")};
    if (!std::isfinite(width) || !std::isfinite(height) || width <= 0.0 || height <= 0.0) {
        return {false, 5205, QStringLiteral("Invalid window size")};
    }
    auto bounds = window->spec_.bounds;
    bounds.width = width;
    bounds.height = height;
    window->spec_.bounds = clamp(bounds);
    return {true, 0, {}};
}

OperationResult SpatialWindowManager::changeDisplayTarget(const QString &windowId, DisplayTarget target, const QString &projectionTarget)
{
    auto *window = find(windowId);
    if (!window) return {false, 5201, QStringLiteral("Window not found")};
    if ((target == DisplayTarget::Projection || target == DisplayTarget::Both) && projectionTarget.isEmpty()) {
        return {false, 5206, QStringLiteral("Projection target is required")};
    }
    window->spec_.displayTarget = target;
    window->spec_.projectionTarget = projectionTarget;
    return {true, 0, {}};
}

OperationResult SpatialWindowManager::restoreDefaultPosition(const QString &windowId)
{
    auto *window = find(windowId);
    if (!window) return {false, 5201, QStringLiteral("Window not found")};
    window->spec_.bounds = defaults_.value(windowId);
    return {true, 0, {}};
}

OperationResult SpatialWindowManager::removeWindow(const QString &windowId)
{
    if (!windows_.remove(windowId)) return {false, 5201, QStringLiteral("Window not found")};
    defaults_.remove(windowId);
    return {true, 0, {}};
}

void SpatialWindowManager::clear()
{
    windows_.clear();
    defaults_.clear();
}

SpatialWindow *SpatialWindowManager::find(const QString &windowId) const { return windows_.value(windowId).get(); }

SpatialWindow *SpatialWindowManager::findByComponentId(const QString &componentId) const
{
    for (const auto &window : windows_) {
        if (window->componentId() == componentId && window->state() != SpatialWindowState::Closed) return window.get();
    }
    return nullptr;
}

QList<SpatialWindow *> SpatialWindowManager::windows() const
{
    QList<SpatialWindow *> result;
    result.reserve(windows_.size());
    for (const auto &window : windows_) result.append(window.get());
    return result;
}

qsizetype SpatialWindowManager::size() const { return windows_.size(); }

qsizetype SpatialWindowManager::openWindowCount() const
{
    return std::count_if(windows_.cbegin(), windows_.cend(), [](const auto &window) {
        return window->state() != SpatialWindowState::Closed;
    });
}

SpatialBounds SpatialWindowManager::clamp(const SpatialBounds &bounds) const
{
    SpatialBounds result = bounds;
    result.width = std::min(result.width, safeArea_.width);
    result.height = std::min(result.height, safeArea_.height);
    result.x = std::clamp(result.x, safeArea_.x, safeArea_.x + safeArea_.width - result.width);
    result.y = std::clamp(result.y, safeArea_.y, safeArea_.y + safeArea_.height - result.height);
    return result;
}

} // namespace astra::ui
