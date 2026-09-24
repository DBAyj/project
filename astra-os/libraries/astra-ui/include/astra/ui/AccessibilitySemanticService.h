#pragma once

#include "astra/ui/Types.h"

#include <QHash>
#include <QStringList>

namespace astra::ui {

enum class SemanticRole { Window, Button, Checkbox, Slider, Label, Alert, Menu, MenuItem, Progress, Image, Model };

struct SemanticNode {
    QString componentId;
    SemanticRole role {SemanticRole::Label};
    QString name;
    QString description;
    QString state;
    QString value;
    QStringList actions;
    bool focusable {false};
    bool visible {true};
};

class AccessibilitySemanticService final {
public:
    OperationResult registerNode(const SemanticNode &node);
    OperationResult removeNode(const QString &componentId);
    OperationResult setVisible(const QString &componentId, bool visible);
    OperationResult invokeAction(const QString &componentId, const QString &action) const;
    [[nodiscard]] QList<SemanticNode> visibleTree() const;
    [[nodiscard]] const SemanticNode *find(const QString &componentId) const;

private:
    QHash<QString, SemanticNode> nodes_;
};

} // namespace astra::ui
