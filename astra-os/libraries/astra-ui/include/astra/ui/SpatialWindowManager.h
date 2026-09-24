#pragma once

#include "astra/ui/Types.h"

#include <QHash>

#include <memory>

namespace astra::ui {

enum class SpatialWindowState { Created, Opening, Visible, Minimized, Hidden, Closing, Closed, Error };

struct SpatialWindowSpec {
    QString windowId;
    QString componentId;
    SpatialBounds bounds;
    DisplayTarget displayTarget {DisplayTarget::Phone};
    QString projectionTarget;
    QString anchorId;
    astra::common::PrivacyLevel privacyLevel {astra::common::PrivacyLevel::PrivateScreenOnly};
    QString focusScope;
};

class SpatialWindow final {
public:
    explicit SpatialWindow(SpatialWindowSpec spec);

    [[nodiscard]] QString windowId() const;
    [[nodiscard]] QString componentId() const;
    [[nodiscard]] SpatialBounds bounds() const;
    [[nodiscard]] SpatialWindowState state() const;
    [[nodiscard]] DisplayTarget displayTarget() const;
    [[nodiscard]] QString projectionTarget() const;
    [[nodiscard]] QString anchorId() const;
    [[nodiscard]] astra::common::PrivacyLevel privacyLevel() const;
    [[nodiscard]] QString focusScope() const;

private:
    friend class SpatialWindowManager;
    SpatialWindowSpec spec_;
    SpatialWindowState state_ {SpatialWindowState::Created};
};

class SpatialWindowManager final {
public:
    SpatialWindowManager(qsizetype maximumWindows = 20, SpatialBounds safeArea = {0.0, 0.0, 1280.0, 720.0});

    OperationResult createWindow(const SpatialWindowSpec &spec);
    OperationResult showWindow(const QString &windowId);
    OperationResult hideWindow(const QString &windowId);
    OperationResult closeWindow(const QString &windowId);
    OperationResult moveWindow(const QString &windowId, double x, double y);
    OperationResult resizeWindow(const QString &windowId, double width, double height);
    OperationResult changeDisplayTarget(const QString &windowId, DisplayTarget target, const QString &projectionTarget = {});
    OperationResult restoreDefaultPosition(const QString &windowId);
    void clear();
    [[nodiscard]] SpatialWindow *find(const QString &windowId) const;
    [[nodiscard]] SpatialWindow *findByComponentId(const QString &componentId) const;
    [[nodiscard]] QList<SpatialWindow *> windows() const;
    [[nodiscard]] qsizetype size() const;

private:
    [[nodiscard]] SpatialBounds clamp(const SpatialBounds &bounds) const;

    qsizetype maximumWindows_;
    SpatialBounds safeArea_;
    QHash<QString, std::shared_ptr<SpatialWindow>> windows_;
    QHash<QString, SpatialBounds> defaults_;
};

} // namespace astra::ui
