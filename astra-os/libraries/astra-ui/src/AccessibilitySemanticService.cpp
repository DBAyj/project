#include "astra/ui/AccessibilitySemanticService.h"

#include <algorithm>

namespace astra::ui {

OperationResult AccessibilitySemanticService::registerNode(const SemanticNode &node)
{
    if (node.componentId.isEmpty() || node.name.isEmpty() || (node.focusable && node.actions.isEmpty())) {
        return {false, 5901, QStringLiteral("Accessibility semantics are incomplete")};
    }
    nodes_.insert(node.componentId, node);
    return {true, 0, {}};
}

OperationResult AccessibilitySemanticService::removeNode(const QString &componentId)
{
    if (nodes_.remove(componentId) == 0) return {false, 5501, QStringLiteral("Semantic node not found")};
    return {true, 0, {}};
}

OperationResult AccessibilitySemanticService::setVisible(const QString &componentId, bool visible)
{
    auto iterator = nodes_.find(componentId);
    if (iterator == nodes_.end()) return {false, 5501, QStringLiteral("Semantic node not found")};
    iterator->visible = visible;
    return {true, 0, {}};
}

OperationResult AccessibilitySemanticService::invokeAction(const QString &componentId, const QString &action) const
{
    const auto node = nodes_.value(componentId);
    if (node.componentId.isEmpty() || !node.visible) return {false, 5501, QStringLiteral("Semantic node not found")};
    if (!node.actions.contains(action)) return {false, 5902, QStringLiteral("Accessibility action is unsupported")};
    return {true, 0, {}};
}

QList<SemanticNode> AccessibilitySemanticService::visibleTree() const
{
    QList<SemanticNode> result;
    for (const auto &node : nodes_) {
        if (node.visible) result.append(node);
    }
    std::sort(result.begin(), result.end(), [](const SemanticNode &left, const SemanticNode &right) { return left.componentId < right.componentId; });
    return result;
}

const SemanticNode *AccessibilitySemanticService::find(const QString &componentId) const
{
    const auto iterator = nodes_.constFind(componentId);
    return iterator == nodes_.constEnd() ? nullptr : &iterator.value();
}

} // namespace astra::ui
