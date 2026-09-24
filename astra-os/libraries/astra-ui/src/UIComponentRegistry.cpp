#include "astra/ui/UIComponentRegistry.h"

namespace astra::ui {

UIComponentRegistry::UIComponentRegistry(qsizetype maximumComponents)
    : maximumComponents_(maximumComponents)
{
}

ComponentCreateResult UIComponentRegistry::create(const ComponentSpec &spec, Principal principal)
{
    if (spec.id.isEmpty() || !spec.bounds.isValid() || spec.accessibilityLabel.isEmpty()) {
        return {{false, 5103, QStringLiteral("Invalid component specification")}, nullptr};
    }
    if (components_.contains(spec.id)) return {{false, 5103, QStringLiteral("Component id already exists")}, nullptr};
    if (isSystemOnly(spec.type) && principal != Principal::System) {
        return {{false, 5104, QStringLiteral("System component type is protected")}, nullptr};
    }
    if (components_.size() >= maximumComponents_) return {{false, 5106, QStringLiteral("Component limit exceeded")}, nullptr};
    auto parent = spec.parentId.isEmpty() ? std::shared_ptr<SpatialUIComponent> {} : components_.value(spec.parentId);
    if ((!spec.parentId.isEmpty() && !parent) || spec.parentId == spec.id) {
        return {{false, 5103, QStringLiteral("Invalid component parent")}, nullptr};
    }
    QStringList uniqueChildren;
    for (const auto &childId : spec.children) {
        const auto child = components_.value(childId);
        bool childIsAncestor = false;
        for (auto ancestor = parent; ancestor; ancestor = components_.value(ancestor->parentId())) {
            if (ancestor->componentId() == childId) {
                childIsAncestor = true;
                break;
            }
        }
        if (!child || childId == spec.id || uniqueChildren.contains(childId)
            || childIsAncestor || (!child->parentId().isEmpty() && child->parentId() != spec.id)) {
            return {{false, 5103, QStringLiteral("Invalid component hierarchy")}, nullptr};
        }
        uniqueChildren.append(childId);
    }
    auto component = std::make_shared<SpatialUIComponent>(spec);
    auto *raw = component.get();
    components_.insert(spec.id, std::move(component));
    if (parent) static_cast<void>(parent->addChild(spec.id));
    for (const auto &childId : uniqueChildren) {
        static_cast<void>(components_.value(childId)->setParentId(spec.id));
    }
    return {{true, 0, {}}, raw};
}

OperationResult UIComponentRegistry::remove(const QString &componentId)
{
    auto component = components_.value(componentId);
    if (!component) return {false, 5105, QStringLiteral("Component not found")};
    if (component->lifecycleState() != ComponentLifecycleState::Destroyed) {
        return {false, 5107, QStringLiteral("Only destroyed components may be removed")};
    }
    if (auto parent = components_.value(component->parentId())) parent->removeChild(componentId);
    for (const auto &childId : component->children()) {
        if (auto child = components_.value(childId)) static_cast<void>(child->setParentId({}));
    }
    components_.remove(componentId);
    return {true, 0, {}};
}

void UIComponentRegistry::clear() { components_.clear(); }

SpatialUIComponent *UIComponentRegistry::find(const QString &componentId) const { return components_.value(componentId).get(); }

QList<SpatialUIComponent *> UIComponentRegistry::components() const
{
    QList<SpatialUIComponent *> result;
    result.reserve(components_.size());
    for (const auto &component : components_) result.append(component.get());
    return result;
}

qsizetype UIComponentRegistry::size() const { return components_.size(); }

} // namespace astra::ui
