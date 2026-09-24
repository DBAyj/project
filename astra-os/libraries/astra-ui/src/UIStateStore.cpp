#include "astra/ui/UIStateStore.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSaveFile>
#include <QSet>

namespace astra::ui {
namespace {

QString layoutName(LayoutMode mode)
{
    switch (mode) {
    case LayoutMode::Stack: return QStringLiteral("STACK");
    case LayoutMode::Grid: return QStringLiteral("GRID");
    case LayoutMode::Radial: return QStringLiteral("RADIAL");
    case LayoutMode::Freeform: return QStringLiteral("FREEFORM");
    case LayoutMode::AnchorRelative: return QStringLiteral("ANCHOR_RELATIVE");
    }
    return QStringLiteral("STACK");
}

std::optional<LayoutMode> parseLayout(const QString &value)
{
    if (value == QStringLiteral("STACK")) return LayoutMode::Stack;
    if (value == QStringLiteral("GRID")) return LayoutMode::Grid;
    if (value == QStringLiteral("RADIAL")) return LayoutMode::Radial;
    if (value == QStringLiteral("FREEFORM")) return LayoutMode::Freeform;
    if (value == QStringLiteral("ANCHOR_RELATIVE")) return LayoutMode::AnchorRelative;
    return std::nullopt;
}

QString componentTypeName(ComponentType type)
{
    switch (type) {
    case ComponentType::SpatialWindow: return QStringLiteral("SPATIAL_WINDOW");
    case ComponentType::TaskCard: return QStringLiteral("TASK_CARD");
    case ComponentType::SystemPanel: return QStringLiteral("SYSTEM_PANEL");
    case ComponentType::Notification: return QStringLiteral("NOTIFICATION");
    case ComponentType::Button: return QStringLiteral("BUTTON");
    case ComponentType::Toggle: return QStringLiteral("TOGGLE");
    case ComponentType::Slider: return QStringLiteral("SLIDER");
    case ComponentType::Label: return QStringLiteral("LABEL");
    case ComponentType::Image: return QStringLiteral("IMAGE");
    case ComponentType::ModelView: return QStringLiteral("MODEL_VIEW");
    case ComponentType::PrivacyBadge: return QStringLiteral("PRIVACY_BADGE");
    case ComponentType::ContextMenu: return QStringLiteral("CONTEXT_MENU");
    case ComponentType::DebugPanel: return QStringLiteral("DEBUG_PANEL");
    case ComponentType::SystemSecurityOverlay: return QStringLiteral("SYSTEM_SECURITY_OVERLAY");
    case ComponentType::PrivacyMask: return QStringLiteral("PRIVACY_MASK");
    case ComponentType::SystemCriticalAlert: return QStringLiteral("SYSTEM_CRITICAL_ALERT");
    }
    return QStringLiteral("LABEL");
}

std::optional<ComponentType> parseComponentType(const QString &value)
{
    for (const auto type : {ComponentType::SpatialWindow, ComponentType::TaskCard, ComponentType::SystemPanel, ComponentType::Notification,
                            ComponentType::Button, ComponentType::Toggle, ComponentType::Slider, ComponentType::Label, ComponentType::Image,
                            ComponentType::ModelView, ComponentType::PrivacyBadge, ComponentType::ContextMenu, ComponentType::DebugPanel}) {
        if (componentTypeName(type) == value) return type;
    }
    return std::nullopt;
}

QString displayTargetName(DisplayTarget target)
{
    if (target == DisplayTarget::Phone) return QStringLiteral("PHONE");
    if (target == DisplayTarget::Projection) return QStringLiteral("PROJECTION");
    return QStringLiteral("BOTH");
}

std::optional<DisplayTarget> parseDisplayTarget(const QString &value)
{
    if (value == QStringLiteral("PHONE")) return DisplayTarget::Phone;
    if (value == QStringLiteral("PROJECTION")) return DisplayTarget::Projection;
    if (value == QStringLiteral("BOTH")) return DisplayTarget::Both;
    return std::nullopt;
}

bool exactKeys(const QJsonObject &object, const QSet<QString> &keys)
{
    if (object.size() != keys.size()) return false;
    for (auto iterator = object.begin(); iterator != object.end(); ++iterator) {
        if (!keys.contains(iterator.key())) return false;
    }
    return true;
}

QJsonObject encodeBounds(const SpatialBounds &bounds)
{
    return {{QStringLiteral("x"), bounds.x}, {QStringLiteral("y"), bounds.y}, {QStringLiteral("width"), bounds.width}, {QStringLiteral("height"), bounds.height}};
}

std::optional<SpatialBounds> decodeBounds(const QJsonObject &object)
{
    if (!exactKeys(object, {QStringLiteral("x"), QStringLiteral("y"), QStringLiteral("width"), QStringLiteral("height")})) return std::nullopt;
    SpatialBounds bounds {object.value(QStringLiteral("x")).toDouble(), object.value(QStringLiteral("y")).toDouble(),
                          object.value(QStringLiteral("width")).toDouble(), object.value(QStringLiteral("height")).toDouble()};
    return bounds.isValid() ? std::optional<SpatialBounds> {bounds} : std::nullopt;
}

} // namespace

UIStateStore::UIStateStore(QString path)
    : path_(std::move(path))
{
}

OperationResult UIStateStore::save(const UIStateSnapshot &snapshot) const
{
    QJsonArray components;
    for (const auto &component : snapshot.components) {
        if (component.componentId.isEmpty() || !component.bounds.isValid() || isSystemOnly(component.componentType)) {
            return {false, 5803, QStringLiteral("Invalid component state")};
        }
        components.append(QJsonObject {
            {QStringLiteral("component_id"), component.componentId}, {QStringLiteral("component_type"), componentTypeName(component.componentType)},
            {QStringLiteral("bounds"), encodeBounds(component.bounds)}, {QStringLiteral("z_order"), component.zOrder},
            {QStringLiteral("visible"), component.visible}, {QStringLiteral("focusable"), component.focusable},
            {QStringLiteral("interactive"), component.interactive},
            {QStringLiteral("display_target"), displayTargetName(component.displayTarget)},
        });
    }
    QJsonArray windows;
    for (const auto &window : snapshot.windows) {
        if (window.windowId.isEmpty() || window.componentId.isEmpty() || !window.bounds.isValid() || window.focusScope.isEmpty()) {
            return {false, 5803, QStringLiteral("Invalid window state")};
        }
        windows.append(QJsonObject {
            {QStringLiteral("window_id"), window.windowId}, {QStringLiteral("component_id"), window.componentId},
            {QStringLiteral("bounds"), encodeBounds(window.bounds)}, {QStringLiteral("visible"), window.visible},
            {QStringLiteral("display_target"), displayTargetName(window.displayTarget)},
            {QStringLiteral("projection_target"), window.projectionTarget.isEmpty() ? QJsonValue {QJsonValue::Null} : QJsonValue {window.projectionTarget}},
            {QStringLiteral("anchor_id"), window.anchorId.isEmpty() ? QJsonValue {QJsonValue::Null} : QJsonValue {window.anchorId}},
            {QStringLiteral("focus_scope"), window.focusScope},
        });
    }
    QJsonArray visible;
    for (const auto &id : snapshot.visibleComponentIds) visible.append(id);
    QJsonArray panels;
    for (const auto &panel : snapshot.panelOrder) panels.append(panel);
    const QJsonObject state {{QStringLiteral("schema_version"), QStringLiteral("1.1")}, {QStringLiteral("layout_mode"), layoutName(snapshot.layoutMode)},
                             {QStringLiteral("components"), components}, {QStringLiteral("windows"), windows}, {QStringLiteral("visible_component_ids"), visible},
                             {QStringLiteral("selected_tab"), snapshot.selectedTab}, {QStringLiteral("focus_restore_component_id"), snapshot.focusRestoreComponentId},
                             {QStringLiteral("panel_order"), panels},
                             {QStringLiteral("saved_at"), QDateTime::currentDateTimeUtc().toString(Qt::ISODateWithMs)}};
    if (!QDir {}.mkpath(QFileInfo {path_}.absolutePath())) return {false, 5801, QStringLiteral("State directory creation failed")};
    QSaveFile file {path_};
    if (!file.open(QIODevice::WriteOnly) || file.write(QJsonDocument {state}.toJson(QJsonDocument::Indented)) < 0 || !file.commit()) {
        return {false, 5801, QStringLiteral("State save failed")};
    }
    return {true, 0, {}};
}

UIStateLoadResult UIStateStore::load() const
{
    QFile file {path_};
    if (!file.open(QIODevice::ReadOnly)) return {{false, 5802, QStringLiteral("State load failed")}, {}};
    QJsonParseError parseError;
    const QJsonDocument document = QJsonDocument::fromJson(file.readAll(), &parseError);
    if (parseError.error != QJsonParseError::NoError || !document.isObject()) return {{false, 5803, QStringLiteral("State JSON is invalid")}, {}};
    const QJsonObject state = document.object();
    const QSet<QString> rootKeys {QStringLiteral("schema_version"), QStringLiteral("layout_mode"), QStringLiteral("components"), QStringLiteral("windows"),
                                  QStringLiteral("visible_component_ids"), QStringLiteral("selected_tab"),
                                  QStringLiteral("focus_restore_component_id"), QStringLiteral("panel_order"), QStringLiteral("saved_at")};
    if (!exactKeys(state, rootKeys)) return {{false, 5803, QStringLiteral("State schema is invalid")}, {}};
    if (state.value(QStringLiteral("schema_version")).toString() != QStringLiteral("1.1")) return {{false, 5804, QStringLiteral("State version is incompatible")}, {}};
    const auto layout = parseLayout(state.value(QStringLiteral("layout_mode")).toString());
    if (!layout) return {{false, 5803, QStringLiteral("State layout is invalid")}, {}};
    UIStateSnapshot snapshot;
    snapshot.layoutMode = *layout;
    for (const auto &value : state.value(QStringLiteral("components")).toArray()) {
        if (!value.isObject()) return {{false, 5803, QStringLiteral("Component state is invalid")}, {}};
        const QJsonObject component = value.toObject();
        const QSet<QString> keys {QStringLiteral("component_id"), QStringLiteral("component_type"), QStringLiteral("bounds"),
                                  QStringLiteral("z_order"), QStringLiteral("visible"), QStringLiteral("focusable"),
                                  QStringLiteral("interactive"), QStringLiteral("display_target")};
        if (!exactKeys(component, keys)) return {{false, 5803, QStringLiteral("Component state is invalid")}, {}};
        const auto type = parseComponentType(component.value(QStringLiteral("component_type")).toString());
        const auto bounds = decodeBounds(component.value(QStringLiteral("bounds")).toObject());
        const auto target = parseDisplayTarget(component.value(QStringLiteral("display_target")).toString());
        if (component.value(QStringLiteral("component_id")).toString().isEmpty() || !type || !bounds || !target
            || !component.value(QStringLiteral("z_order")).isDouble() || !component.value(QStringLiteral("visible")).isBool()
            || !component.value(QStringLiteral("focusable")).isBool() || !component.value(QStringLiteral("interactive")).isBool()) {
            return {{false, 5803, QStringLiteral("Component state is invalid")}, {}};
        }
        snapshot.components.append({component.value(QStringLiteral("component_id")).toString(), *type, *bounds,
                                    component.value(QStringLiteral("z_order")).toInt(), component.value(QStringLiteral("visible")).toBool(),
                                    component.value(QStringLiteral("focusable")).toBool(), component.value(QStringLiteral("interactive")).toBool(), *target});
    }
    for (const auto &value : state.value(QStringLiteral("windows")).toArray()) {
        if (!value.isObject()) return {{false, 5803, QStringLiteral("Window state is invalid")}, {}};
        const QJsonObject window = value.toObject();
        const QSet<QString> keys {QStringLiteral("window_id"), QStringLiteral("component_id"), QStringLiteral("bounds"), QStringLiteral("visible"),
                                  QStringLiteral("display_target"), QStringLiteral("projection_target"), QStringLiteral("anchor_id"),
                                  QStringLiteral("focus_scope")};
        if (!exactKeys(window, keys)) return {{false, 5803, QStringLiteral("Window state is invalid")}, {}};
        const auto bounds = decodeBounds(window.value(QStringLiteral("bounds")).toObject());
        const QString id = window.value(QStringLiteral("window_id")).toString();
        const auto target = parseDisplayTarget(window.value(QStringLiteral("display_target")).toString());
        if (!bounds || id.isEmpty() || window.value(QStringLiteral("component_id")).toString().isEmpty() || !target
            || !window.value(QStringLiteral("visible")).isBool() || window.value(QStringLiteral("focus_scope")).toString().isEmpty()) {
            return {{false, 5803, QStringLiteral("Window state is invalid")}, {}};
        }
        snapshot.windows.append({id, window.value(QStringLiteral("component_id")).toString(), *bounds,
                                 window.value(QStringLiteral("visible")).toBool(), *target,
                                 window.value(QStringLiteral("projection_target")).toString(), window.value(QStringLiteral("anchor_id")).toString(),
                                 window.value(QStringLiteral("focus_scope")).toString()});
    }
    for (const auto &value : state.value(QStringLiteral("visible_component_ids")).toArray()) {
        if (!value.isString() || value.toString().isEmpty()) return {{false, 5803, QStringLiteral("Visible component state is invalid")}, {}};
        snapshot.visibleComponentIds.append(value.toString());
    }
    for (const auto &value : state.value(QStringLiteral("panel_order")).toArray()) {
        if (!value.isString() || value.toString().isEmpty()) return {{false, 5803, QStringLiteral("Panel order is invalid")}, {}};
        snapshot.panelOrder.append(value.toString());
    }
    snapshot.selectedTab = state.value(QStringLiteral("selected_tab")).toString();
    snapshot.focusRestoreComponentId = state.value(QStringLiteral("focus_restore_component_id")).toString();
    return {{true, 0, {}}, snapshot};
}

QString UIStateStore::path() const { return path_; }

} // namespace astra::ui
