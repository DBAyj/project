#pragma once

#include "astra/ui/Types.h"

#include <QDateTime>
#include <QStringList>

namespace astra::ui {

struct ComponentSpec {
    QString id;
    ComponentType type {ComponentType::Label};
    QString parentId;
    QStringList children;
    SpatialBounds bounds;
    SpatialTransform transform;
    int zOrder {0};
    double opacity {1.0};
    bool visible {false};
    bool enabled {true};
    bool focusable {false};
    bool interactive {false};
    astra::common::PrivacyLevel privacyLevel {astra::common::PrivacyLevel::PrivateScreenOnly};
    DisplayTarget displayTarget {DisplayTarget::Phone};
    QString accessibilityLabel;
};

class SpatialUIComponent final {
public:
    explicit SpatialUIComponent(ComponentSpec spec);

    [[nodiscard]] QString componentId() const;
    [[nodiscard]] ComponentType componentType() const;
    [[nodiscard]] QString parentId() const;
    [[nodiscard]] QStringList children() const;
    [[nodiscard]] SpatialBounds bounds() const;
    [[nodiscard]] SpatialTransform transform() const;
    [[nodiscard]] int zOrder() const;
    [[nodiscard]] double opacity() const;
    [[nodiscard]] astra::common::PrivacyLevel privacyLevel() const;
    [[nodiscard]] DisplayTarget displayTarget() const;
    [[nodiscard]] bool isVisible() const;
    [[nodiscard]] bool isEnabled() const;
    [[nodiscard]] bool isFocusable() const;
    [[nodiscard]] bool isInteractive() const;
    [[nodiscard]] QString accessibilityLabel() const;
    [[nodiscard]] ComponentLifecycleState lifecycleState() const;
    [[nodiscard]] QDateTime createdAt() const;
    [[nodiscard]] QDateTime updatedAt() const;

    OperationResult transitionTo(ComponentLifecycleState next);
    OperationResult setBounds(const SpatialBounds &bounds);
    OperationResult setTransform(const SpatialTransform &transform);
    OperationResult setParentId(const QString &parentId);
    OperationResult addChild(const QString &childId);
    void removeChild(const QString &childId);
    void setPrivacyLevel(astra::common::PrivacyLevel level);
    void setDisplayTarget(DisplayTarget target);
    void setAccessibilityLabel(QString label);

private:
    ComponentSpec spec_;
    ComponentLifecycleState lifecycleState_ {ComponentLifecycleState::Created};
    QDateTime createdAt_;
    QDateTime updatedAt_;
};

} // namespace astra::ui
