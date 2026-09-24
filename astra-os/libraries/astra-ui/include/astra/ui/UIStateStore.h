#pragma once

#include "astra/ui/SpatialLayoutEngine.h"

#include <QStringList>

namespace astra::ui {

struct WindowStateSnapshot {
    QString windowId;
    QString componentId;
    SpatialBounds bounds;
    bool visible {true};
    DisplayTarget displayTarget {DisplayTarget::Phone};
    QString projectionTarget;
    QString anchorId;
    QString focusScope;
};

struct ComponentStateSnapshot {
    QString componentId;
    ComponentType componentType {ComponentType::Label};
    SpatialBounds bounds;
    int zOrder {0};
    bool visible {true};
    bool focusable {false};
    bool interactive {false};
    DisplayTarget displayTarget {DisplayTarget::Phone};
};

struct UIStateSnapshot {
    LayoutMode layoutMode {LayoutMode::Stack};
    QList<ComponentStateSnapshot> components;
    QList<WindowStateSnapshot> windows;
    QStringList visibleComponentIds;
    QString selectedTab;
    QString focusRestoreComponentId;
    QStringList panelOrder;
};

struct UIStateLoadResult : OperationResult {
    UIStateSnapshot snapshot;
};

class UIStateStore final {
public:
    explicit UIStateStore(QString path);
    OperationResult save(const UIStateSnapshot &snapshot) const;
    [[nodiscard]] UIStateLoadResult load() const;
    [[nodiscard]] QString path() const;

private:
    QString path_;
};

} // namespace astra::ui
