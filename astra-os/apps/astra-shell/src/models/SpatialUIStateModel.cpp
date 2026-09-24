#include "models/SpatialUIStateModel.h"

#include <QJsonArray>
#include <QHash>
#include <QSet>

#include <functional>

namespace astra::shell {

SpatialUIStateModel::SpatialUIStateModel(QObject *parent)
    : QObject(parent)
{
}

QString SpatialUIStateModel::status() const { return status_; }
int SpatialUIStateModel::componentCount() const { return componentCount_; }
int SpatialUIStateModel::windowCount() const { return windowCount_; }
QString SpatialUIStateModel::focusComponentId() const { return focusComponentId_; }
bool SpatialUIStateModel::projectionSafe() const { return projectionSafe_; }
bool SpatialUIStateModel::reduceMotion() const { return reduceMotion_; }
bool SpatialUIStateModel::highContrast() const { return highContrast_; }
QString SpatialUIStateModel::p4ReleaseStatus() const { return p4ReleaseStatus_; }
QVariantList SpatialUIStateModel::components() const { return components_; }

QVariantList SpatialUIStateModel::componentsForDisplay(const QString &displayTarget) const
{
    QHash<QString, QVariantMap> eligible;
    QStringList order;
    for (const auto &componentValue : components_) {
        const auto component = componentValue.toMap();
        const QString target = component.value(QStringLiteral("display_target")).toString();
        if (target != displayTarget && target != QStringLiteral("BOTH")) continue;
        if (displayTarget == QStringLiteral("PROJECTION")
            && component.value(QStringLiteral("privacy_level")).toString() != QStringLiteral("PUBLIC")) {
            continue;
        }
        const QString componentId = component.value(QStringLiteral("component_id")).toString();
        eligible.insert(componentId, component);
        order.append(componentId);
    }
    QVariantList roots;
    QSet<QString> visited;
    std::function<QVariantMap(const QString &, int)> buildTree = [&](const QString &componentId, int depth) {
        if (visited.contains(componentId) || !eligible.contains(componentId)) return QVariantMap {};
        visited.insert(componentId);
        auto component = eligible.value(componentId);
        component.insert(QStringLiteral("hierarchy_depth"), depth);
        QVariantList children;
        for (const auto &childValue : component.value(QStringLiteral("children")).toList()) {
            const auto child = buildTree(childValue.toString(), depth + 1);
            if (!child.isEmpty()) children.append(child);
        }
        for (const auto &candidateId : order) {
            if (eligible.value(candidateId).value(QStringLiteral("parent_id")).toString() == componentId) {
                const auto child = buildTree(candidateId, depth + 1);
                if (!child.isEmpty()) children.append(child);
            }
        }
        component.insert(QStringLiteral("child_components"), children);
        return component;
    };
    for (const auto &componentId : order) {
        const QString parentId = eligible.value(componentId).value(QStringLiteral("parent_id")).toString();
        if (parentId.isEmpty() || !eligible.contains(parentId)) {
            const auto root = buildTree(componentId, 0);
            if (!root.isEmpty()) roots.append(root);
        }
    }
    for (const auto &componentId : order) {
        const auto root = buildTree(componentId, 0);
        if (!root.isEmpty()) roots.append(root);
    }
    return roots;
}

void SpatialUIStateModel::applyStatus(const QJsonObject &status)
{
    status_ = status.value(QStringLiteral("status")).toString(QStringLiteral("READY"));
    componentCount_ = status.value(QStringLiteral("component_count")).toInt(componentCount_);
    windowCount_ = status.value(QStringLiteral("window_count")).toInt(windowCount_);
    focusComponentId_ = status.value(QStringLiteral("focus_component_id")).toString();
    projectionSafe_ = status.value(QStringLiteral("projection_safe")).toBool(projectionSafe_);
    reduceMotion_ = status.value(QStringLiteral("reduce_motion")).toBool(reduceMotion_);
    highContrast_ = status.value(QStringLiteral("high_contrast")).toBool(highContrast_);
    p4ReleaseStatus_ = status.value(QStringLiteral("p4_release_status")).toString(p4ReleaseStatus_);
    emit changed();
}

void SpatialUIStateModel::applyComponents(const QJsonObject &result)
{
    components_ = result.value(QStringLiteral("components")).toArray().toVariantList();
    componentCount_ = result.value(QStringLiteral("component_count")).toInt(components_.size());
    emit changed();
}

void SpatialUIStateModel::setOffline()
{
    status_ = QStringLiteral("OFFLINE");
    projectionSafe_ = true;
    components_.clear();
    componentCount_ = 0;
    emit changed();
}

} // namespace astra::shell
