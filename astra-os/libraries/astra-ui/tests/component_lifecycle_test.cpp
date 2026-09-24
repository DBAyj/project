#include "astra/ui/SpatialUIComponent.h"
#include "astra/ui/UIComponentRegistry.h"

#include <QtGlobal>

using namespace astra::ui;

namespace {
void require(bool condition)
{
    if (!condition) qFatal("P5 component lifecycle requirement failed");
}
}

int main()
{
    UIComponentRegistry registry {2};
    ComponentSpec card;
    card.id = QStringLiteral("card-1");
    card.type = ComponentType::TaskCard;
    card.bounds = {10.0, 20.0, 320.0, 180.0};
    card.privacyLevel = astra::common::PrivacyLevel::Public;
    card.accessibilityLabel = QStringLiteral("Task card");

    const auto created = registry.create(card, Principal::Application);
    require(created.ok);
    require(created.component != nullptr);
    require(registry.size() == 1);
    require(!registry.create(card, Principal::Application).ok);

    auto *component = created.component;
    require(component->lifecycleState() == ComponentLifecycleState::Created);
    require(component->transitionTo(ComponentLifecycleState::Attached).ok);
    require(component->transitionTo(ComponentLifecycleState::Visible).ok);
    require(!component->transitionTo(ComponentLifecycleState::Destroyed).ok);
    require(component->lifecycleState() == ComponentLifecycleState::Visible);
    require(component->transitionTo(ComponentLifecycleState::Focused).ok);
    require(component->transitionTo(ComponentLifecycleState::Interacting).ok);
    require(component->transitionTo(ComponentLifecycleState::Focused).ok);

    ComponentSpec protectedOverlay = card;
    protectedOverlay.id = QStringLiteral("security-overlay");
    protectedOverlay.type = ComponentType::SystemSecurityOverlay;
    require(!registry.create(protectedOverlay, Principal::Application).ok);
    require(registry.create(protectedOverlay, Principal::System).ok);

    ComponentSpec overLimit = card;
    overLimit.id = QStringLiteral("card-3");
    const auto rejected = registry.create(overLimit, Principal::System);
    require(!rejected.ok);
    require(rejected.errorCode == 5106);

    UIComponentRegistry hierarchyRegistry {4};
    ComponentSpec parent = card;
    parent.id = QStringLiteral("parent");
    require(hierarchyRegistry.create(parent, Principal::Application).ok);
    ComponentSpec child = card;
    child.id = QStringLiteral("child");
    child.parentId = parent.id;
    const auto childCreated = hierarchyRegistry.create(child, Principal::Application);
    require(childCreated.ok);
    require(childCreated.component->parentId() == parent.id);
    require(hierarchyRegistry.find(parent.id)->children() == QStringList {child.id});
    ComponentSpec cycle = card;
    cycle.id = QStringLiteral("cycle");
    cycle.parentId = child.id;
    cycle.children = {parent.id};
    require(!hierarchyRegistry.create(cycle, Principal::Application).ok);
    require(childCreated.component->transitionTo(ComponentLifecycleState::Attached).ok);
    require(childCreated.component->transitionTo(ComponentLifecycleState::Visible).ok);
    require(childCreated.component->transitionTo(ComponentLifecycleState::Hidden).ok);
    require(childCreated.component->transitionTo(ComponentLifecycleState::Detached).ok);
    require(childCreated.component->transitionTo(ComponentLifecycleState::Destroyed).ok);
    require(hierarchyRegistry.remove(child.id).ok);
    require(hierarchyRegistry.find(parent.id)->children().isEmpty());
}
