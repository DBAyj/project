#pragma once

#include "astra/ui/SpatialUIComponent.h"

#include <QHash>

#include <memory>

namespace astra::ui {

struct ComponentCreateResult : OperationResult {
    SpatialUIComponent *component {nullptr};
};

class UIComponentRegistry final {
public:
    explicit UIComponentRegistry(qsizetype maximumComponents = 500);

    ComponentCreateResult create(const ComponentSpec &spec, Principal principal);
    OperationResult remove(const QString &componentId);
    void clear();
    [[nodiscard]] SpatialUIComponent *find(const QString &componentId) const;
    [[nodiscard]] QList<SpatialUIComponent *> components() const;
    [[nodiscard]] qsizetype size() const;

private:
    qsizetype maximumComponents_;
    QHash<QString, std::shared_ptr<SpatialUIComponent>> components_;
};

} // namespace astra::ui
