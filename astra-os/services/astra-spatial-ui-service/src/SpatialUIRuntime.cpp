#include "astra/spatial_ui/service/SpatialUIRuntime.h"

#include "astra/common/PrivacyLevel.h"
#include "astra/policy/ProjectionPolicyService.h"

#include <QJsonArray>
#include <QDateTime>
#include <QSet>
#include <QUuid>

#include <algorithm>
#include <cmath>

namespace astra::spatial_ui::service {
namespace {

bool exactKeys(const QJsonObject &object, const QSet<QString> &keys)
{
    if (object.size() != keys.size()) return false;
    for (auto iterator = object.begin(); iterator != object.end(); ++iterator) {
        if (!keys.contains(iterator.key())) return false;
    }
    return true;
}

bool isUuid(const QString &value)
{
    return !value.isEmpty() && !QUuid {value}.isNull();
}

std::optional<astra::ui::ComponentType> componentType(const QString &value)
{
    using Type = astra::ui::ComponentType;
    if (value == QStringLiteral("SPATIAL_WINDOW")) return Type::SpatialWindow;
    if (value == QStringLiteral("TASK_CARD")) return Type::TaskCard;
    if (value == QStringLiteral("SYSTEM_PANEL")) return Type::SystemPanel;
    if (value == QStringLiteral("NOTIFICATION")) return Type::Notification;
    if (value == QStringLiteral("BUTTON")) return Type::Button;
    if (value == QStringLiteral("TOGGLE")) return Type::Toggle;
    if (value == QStringLiteral("SLIDER")) return Type::Slider;
    if (value == QStringLiteral("LABEL")) return Type::Label;
    if (value == QStringLiteral("IMAGE")) return Type::Image;
    if (value == QStringLiteral("MODEL_VIEW")) return Type::ModelView;
    if (value == QStringLiteral("PRIVACY_BADGE")) return Type::PrivacyBadge;
    if (value == QStringLiteral("CONTEXT_MENU")) return Type::ContextMenu;
    if (value == QStringLiteral("DEBUG_PANEL")) return Type::DebugPanel;
    return std::nullopt;
}

QString componentTypeName(astra::ui::ComponentType type)
{
    using Type = astra::ui::ComponentType;
    switch (type) {
    case Type::SpatialWindow: return QStringLiteral("SPATIAL_WINDOW");
    case Type::TaskCard: return QStringLiteral("TASK_CARD");
    case Type::SystemPanel: return QStringLiteral("SYSTEM_PANEL");
    case Type::Notification: return QStringLiteral("NOTIFICATION");
    case Type::Button: return QStringLiteral("BUTTON");
    case Type::Toggle: return QStringLiteral("TOGGLE");
    case Type::Slider: return QStringLiteral("SLIDER");
    case Type::Label: return QStringLiteral("LABEL");
    case Type::Image: return QStringLiteral("IMAGE");
    case Type::ModelView: return QStringLiteral("MODEL_VIEW");
    case Type::PrivacyBadge: return QStringLiteral("PRIVACY_BADGE");
    case Type::ContextMenu: return QStringLiteral("CONTEXT_MENU");
    case Type::DebugPanel: return QStringLiteral("DEBUG_PANEL");
    case Type::SystemSecurityOverlay: return QStringLiteral("SYSTEM_SECURITY_OVERLAY");
    case Type::PrivacyMask: return QStringLiteral("PRIVACY_MASK");
    case Type::SystemCriticalAlert: return QStringLiteral("SYSTEM_CRITICAL_ALERT");
    }
    return {};
}

QJsonObject encodeBounds(const astra::ui::SpatialBounds &value)
{
    return {{QStringLiteral("x"), value.x}, {QStringLiteral("y"), value.y},
            {QStringLiteral("width"), value.width}, {QStringLiteral("height"), value.height}};
}

QJsonObject encodeTransform(const astra::ui::SpatialTransform &value)
{
    return {{QStringLiteral("translation_x"), value.translationX},
            {QStringLiteral("translation_y"), value.translationY},
            {QStringLiteral("rotation_degrees"), value.rotationDegrees},
            {QStringLiteral("scale"), value.scale}};
}

QString lifecycleStateName(astra::ui::ComponentLifecycleState state)
{
    using State = astra::ui::ComponentLifecycleState;
    switch (state) {
    case State::Created: return QStringLiteral("CREATED");
    case State::Attached: return QStringLiteral("ATTACHED");
    case State::Visible: return QStringLiteral("VISIBLE");
    case State::Focused: return QStringLiteral("FOCUSED");
    case State::Interacting: return QStringLiteral("INTERACTING");
    case State::Hidden: return QStringLiteral("HIDDEN");
    case State::Suspended: return QStringLiteral("SUSPENDED");
    case State::Detached: return QStringLiteral("DETACHED");
    case State::Destroyed: return QStringLiteral("DESTROYED");
    case State::Error: return QStringLiteral("ERROR");
    }
    return {};
}

QString windowStateName(astra::ui::SpatialWindowState state)
{
    using State = astra::ui::SpatialWindowState;
    switch (state) {
    case State::Created: return QStringLiteral("CREATED");
    case State::Opening: return QStringLiteral("OPENING");
    case State::Visible: return QStringLiteral("VISIBLE");
    case State::Minimized: return QStringLiteral("MINIMIZED");
    case State::Hidden: return QStringLiteral("HIDDEN");
    case State::Closing: return QStringLiteral("CLOSING");
    case State::Closed: return QStringLiteral("CLOSED");
    case State::Error: return QStringLiteral("ERROR");
    }
    return {};
}

QString focusTypeName(astra::ui::FocusType type)
{
    using Type = astra::ui::FocusType;
    switch (type) {
    case Type::None: return QStringLiteral("NONE");
    case Type::Keyboard: return QStringLiteral("KEYBOARD");
    case Type::Pointer: return QStringLiteral("POINTER");
    case Type::Touch: return QStringLiteral("TOUCH");
    case Type::GestureSimulated: return QStringLiteral("GESTURE_SIMULATED");
    case Type::System: return QStringLiteral("SYSTEM");
    }
    return {};
}

QString interactionStateName(astra::ui::InteractionState state)
{
    using State = astra::ui::InteractionState;
    switch (state) {
    case State::Idle: return QStringLiteral("IDLE");
    case State::Hovered: return QStringLiteral("HOVERED");
    case State::Pressed: return QStringLiteral("PRESSED");
    case State::Selected: return QStringLiteral("SELECTED");
    case State::Dragging: return QStringLiteral("DRAGGING");
    case State::Resizing: return QStringLiteral("RESIZING");
    case State::Rotating: return QStringLiteral("ROTATING");
    case State::Scaling: return QStringLiteral("SCALING");
    case State::Disabled: return QStringLiteral("DISABLED");
    case State::Cancelled: return QStringLiteral("CANCELLED");
    case State::Error: return QStringLiteral("ERROR");
    }
    return QStringLiteral("ERROR");
}

QString taskSurfaceStateName(astra::ui::TaskSurfaceState state)
{
    using State = astra::ui::TaskSurfaceState;
    switch (state) {
    case State::Created: return QStringLiteral("CREATED");
    case State::WaitingConfirmation: return QStringLiteral("WAITING_CONFIRMATION");
    case State::Ready: return QStringLiteral("READY");
    case State::Running: return QStringLiteral("RUNNING");
    case State::Paused: return QStringLiteral("PAUSED");
    case State::Completed: return QStringLiteral("COMPLETED");
    case State::Failed: return QStringLiteral("FAILED");
    case State::Cancelled: return QStringLiteral("CANCELLED");
    }
    return QStringLiteral("CREATED");
}

QString notificationSeverityName(astra::ui::NotificationSeverity severity)
{
    using Severity = astra::ui::NotificationSeverity;
    switch (severity) {
    case Severity::Info: return QStringLiteral("INFO");
    case Severity::Success: return QStringLiteral("SUCCESS");
    case Severity::Warning: return QStringLiteral("WARNING");
    case Severity::Error: return QStringLiteral("ERROR");
    case Severity::Critical: return QStringLiteral("CRITICAL");
    }
    return QStringLiteral("INFO");
}

std::optional<astra::ui::DisplayTarget> displayTarget(const QString &value)
{
    if (value == QStringLiteral("PHONE")) return astra::ui::DisplayTarget::Phone;
    if (value == QStringLiteral("PROJECTION")) return astra::ui::DisplayTarget::Projection;
    if (value == QStringLiteral("BOTH")) return astra::ui::DisplayTarget::Both;
    return std::nullopt;
}

QString displayTargetName(astra::ui::DisplayTarget target)
{
    if (target == astra::ui::DisplayTarget::Phone) return QStringLiteral("PHONE");
    if (target == astra::ui::DisplayTarget::Projection) return QStringLiteral("PROJECTION");
    return QStringLiteral("BOTH");
}

std::optional<astra::ui::SpatialBounds> bounds(const QJsonObject &value)
{
    if (!exactKeys(value, {QStringLiteral("x"), QStringLiteral("y"), QStringLiteral("width"), QStringLiteral("height")})) return std::nullopt;
    if (!value.value(QStringLiteral("x")).isDouble() || !value.value(QStringLiteral("y")).isDouble()
        || !value.value(QStringLiteral("width")).isDouble() || !value.value(QStringLiteral("height")).isDouble()) {
        return std::nullopt;
    }
    astra::ui::SpatialBounds result {value.value(QStringLiteral("x")).toDouble(), value.value(QStringLiteral("y")).toDouble(),
                                     value.value(QStringLiteral("width")).toDouble(), value.value(QStringLiteral("height")).toDouble()};
    return result.isValid() ? std::optional<astra::ui::SpatialBounds> {result} : std::nullopt;
}

std::optional<astra::ui::InputEventType> inputEventType(const QString &value)
{
    using Type = astra::ui::InputEventType;
    if (value == QStringLiteral("POINTER_MOVE")) return Type::PointerMove;
    if (value == QStringLiteral("POINTER_PRESS")) return Type::PointerPress;
    if (value == QStringLiteral("POINTER_RELEASE")) return Type::PointerRelease;
    if (value == QStringLiteral("POINTER_SCROLL")) return Type::PointerScroll;
    if (value == QStringLiteral("KEY_PRESS")) return Type::KeyPress;
    if (value == QStringLiteral("KEY_RELEASE")) return Type::KeyRelease;
    if (value == QStringLiteral("TOUCH_BEGIN")) return Type::TouchBegin;
    if (value == QStringLiteral("TOUCH_UPDATE")) return Type::TouchUpdate;
    if (value == QStringLiteral("TOUCH_END")) return Type::TouchEnd;
    if (value == QStringLiteral("GESTURE_SELECT")) return Type::GestureSelect;
    if (value == QStringLiteral("GESTURE_GRAB")) return Type::GestureGrab;
    if (value == QStringLiteral("GESTURE_RELEASE")) return Type::GestureRelease;
    if (value == QStringLiteral("GESTURE_SCALE")) return Type::GestureScale;
    if (value == QStringLiteral("GESTURE_ROTATE")) return Type::GestureRotate;
    if (value == QStringLiteral("SYSTEM_FOCUS")) return Type::SystemFocus;
    if (value == QStringLiteral("AI_ACTION")) return Type::AiAction;
    return std::nullopt;
}

std::optional<astra::ui::InputSourceType> inputSourceType(const QString &value)
{
    using Type = astra::ui::InputSourceType;
    if (value == QStringLiteral("MOUSE")) return Type::Mouse;
    if (value == QStringLiteral("KEYBOARD")) return Type::Keyboard;
    if (value == QStringLiteral("TOUCH")) return Type::Touch;
    if (value == QStringLiteral("SIMULATED_GESTURE")) return Type::SimulatedGesture;
    if (value == QStringLiteral("SYSTEM")) return Type::System;
    if (value == QStringLiteral("AI_INTENT")) return Type::AiIntent;
    return std::nullopt;
}

std::optional<astra::ui::CoordinateSystem> coordinateSystem(const QString &value)
{
    using Type = astra::ui::CoordinateSystem;
    if (value == QStringLiteral("PHONE_VIEW")) return Type::PhoneView;
    if (value == QStringLiteral("PROJECTION_VIEW")) return Type::ProjectionView;
    if (value == QStringLiteral("TARGET_LOCAL")) return Type::TargetLocal;
    if (value == QStringLiteral("ANCHOR_LOCAL")) return Type::AnchorLocal;
    return std::nullopt;
}

std::optional<astra::ui::LayoutMode> layoutMode(const QString &value)
{
    using Mode = astra::ui::LayoutMode;
    if (value == QStringLiteral("STACK")) return Mode::Stack;
    if (value == QStringLiteral("GRID")) return Mode::Grid;
    if (value == QStringLiteral("RADIAL")) return Mode::Radial;
    if (value == QStringLiteral("FREEFORM")) return Mode::Freeform;
    if (value == QStringLiteral("ANCHOR_RELATIVE")) return Mode::AnchorRelative;
    return std::nullopt;
}

QString layoutModeName(astra::ui::LayoutMode mode)
{
    using Mode = astra::ui::LayoutMode;
    switch (mode) {
    case Mode::Stack: return QStringLiteral("STACK");
    case Mode::Grid: return QStringLiteral("GRID");
    case Mode::Radial: return QStringLiteral("RADIAL");
    case Mode::Freeform: return QStringLiteral("FREEFORM");
    case Mode::AnchorRelative: return QStringLiteral("ANCHOR_RELATIVE");
    }
    return {};
}

std::optional<astra::ui::LayoutDirection> layoutDirection(const QString &value)
{
    if (value == QStringLiteral("VERTICAL")) return astra::ui::LayoutDirection::Vertical;
    if (value == QStringLiteral("HORIZONTAL")) return astra::ui::LayoutDirection::Horizontal;
    return std::nullopt;
}

std::optional<astra::ui::NotificationSeverity> notificationSeverity(const QString &value)
{
    using Severity = astra::ui::NotificationSeverity;
    if (value == QStringLiteral("INFO")) return Severity::Info;
    if (value == QStringLiteral("SUCCESS")) return Severity::Success;
    if (value == QStringLiteral("WARNING")) return Severity::Warning;
    if (value == QStringLiteral("ERROR")) return Severity::Error;
    if (value == QStringLiteral("CRITICAL")) return Severity::Critical;
    return std::nullopt;
}

bool isSecurityLayer(astra::ui::ComponentType type)
{
    using Type = astra::ui::ComponentType;
    return type == Type::SystemSecurityOverlay || type == Type::PrivacyMask || type == Type::SystemCriticalAlert;
}

} // namespace

namespace {

class DisabledProjectionGateway final : public ProjectionGateway {
public:
    ProjectionGatewayResult submit(const QVector<astra::render::ProjectionLayer> &, const QString &, const QString &) override
    {
        return {{false, 5904, QStringLiteral("P4 projection gateway is not configured")}, 0};
    }
    astra::ui::OperationResult clear(const QString &, const QString &, const QString &) override { return {true, 0, {}}; }
};

} // namespace

SpatialUIRuntime::SpatialUIRuntime(QString statePath,
                                   std::shared_ptr<ProjectionGateway> projectionGateway,
                                   SpatialUIRuntimeOptions options)
    : components_(options.maximumComponents)
    , windows_(options.maximumWindows, {0.0, 0.0, 1280.0, 720.0})
    , layerMapper_(privacy_, options.policyBindingKey)
    , stateStore_(std::move(statePath))
    , projectionGateway_(projectionGateway ? std::move(projectionGateway) : std::make_shared<DisabledProjectionGateway>())
    , options_(std::move(options))
    , layoutMode_(options_.defaultLayout)
{
}

astra::policy::ProjectionPolicyDecision SpatialUIRuntime::classifyFixture(
    const QString &subjectId, astra::common::PrivacyLevel requestedLevel) const
{
    const auto maximumDisclosure = subjectId == options_.publicFixtureSubjectId
        ? astra::common::PrivacyLevel::Public : options_.maximumFixtureDisclosure;
    return astra::policy::ProjectionPolicyService::classifyFixture(
        subjectId, requestedLevel, maximumDisclosure, options_.policyBindingKey);
}

RuntimeResponse SpatialUIRuntime::dispatch(const QString &method, const QJsonObject &params, const QString &traceId, const QString &requestId)
{
    if (method == QStringLiteral("spatial_ui.status")) {
        return params.isEmpty() ? RuntimeResponse {true, 0, {}, status()}
                                : RuntimeResponse {false, 5103, QStringLiteral("Status parameters must be empty"), {}};
    }
    if (method == QStringLiteral("spatial_ui.components")) {
        if (!params.isEmpty()) return {false, 5103, QStringLiteral("Component query parameters must be empty"), {}};
        QJsonArray values;
        for (const auto *component : components_.components()) {
            QJsonObject content;
            if (const auto *task = tasks_.find(component->componentId())) {
                content = {{QStringLiteral("title"), task->title()}, {QStringLiteral("summary"), task->summary()},
                           {QStringLiteral("state"), taskSurfaceStateName(task->state())},
                           {QStringLiteral("progress"), task->progress()}, {QStringLiteral("intent_type"), task->intentType()},
                           {QStringLiteral("confidence"), task->confidence()},
                           {QStringLiteral("execution_strategy"), task->executionStrategy()}};
            } else if (const auto *notification = notifications_.find(component->componentId())) {
                QJsonArray actions;
                for (const auto &action : notification->spec().actions) actions.append(action);
                content = {{QStringLiteral("title"), notification->spec().title},
                           {QStringLiteral("message"), notification->spec().message},
                           {QStringLiteral("severity"), notificationSeverityName(notification->spec().severity)},
                           {QStringLiteral("requires_action"), notification->spec().requiresAction},
                           {QStringLiteral("actions"), actions}};
            }
            QJsonArray children;
            for (const auto &child : component->children()) children.append(child);
            values.append(QJsonObject {{QStringLiteral("component_id"), component->componentId()},
                                       {QStringLiteral("component_type"), componentTypeName(component->componentType())},
                                       {QStringLiteral("parent_id"), component->parentId().isEmpty()
                                            ? QJsonValue {QJsonValue::Null} : QJsonValue {component->parentId()}},
                                       {QStringLiteral("children"), children}, {QStringLiteral("bounds"), encodeBounds(component->bounds())},
                                       {QStringLiteral("transform"), encodeTransform(component->transform())},
                                       {QStringLiteral("z_order"), component->zOrder()}, {QStringLiteral("opacity"), component->opacity()},
                                       {QStringLiteral("visible"), component->isVisible()}, {QStringLiteral("enabled"), component->isEnabled()},
                                       {QStringLiteral("focusable"), component->isFocusable()},
                                       {QStringLiteral("interactive"), component->isInteractive()},
                                       {QStringLiteral("privacy_level"), QString::fromLatin1(astra::common::privacyLevelToString(component->privacyLevel()).data())},
                                       {QStringLiteral("display_target"), displayTargetName(component->displayTarget())},
                                       {QStringLiteral("accessibility_label"), component->accessibilityLabel()},
                                       {QStringLiteral("lifecycle_state"), lifecycleStateName(component->lifecycleState())},
                                       {QStringLiteral("created_at"), component->createdAt().toString(Qt::ISODateWithMs)},
                                       {QStringLiteral("updated_at"), component->updatedAt().toString(Qt::ISODateWithMs)},
                                       {QStringLiteral("content"), content}});
        }
        return {true, 0, {}, {{QStringLiteral("components"), values}, {QStringLiteral("component_count"), values.size()}}};
    }
    if (method == QStringLiteral("spatial_ui.windows")) {
        if (!params.isEmpty()) return {false, 5205, QStringLiteral("Window query parameters must be empty"), {}};
        QJsonArray values;
        for (const auto *window : windows_.windows()) {
            values.append(QJsonObject {{QStringLiteral("window_id"), window->windowId()}, {QStringLiteral("component_id"), window->componentId()},
                                       {QStringLiteral("bounds"), encodeBounds(window->bounds())}, {QStringLiteral("state"), windowStateName(window->state())},
                                       {QStringLiteral("display_target"), displayTargetName(window->displayTarget())},
                                       {QStringLiteral("projection_target"), window->projectionTarget().isEmpty() ? QJsonValue {QJsonValue::Null} : QJsonValue {window->projectionTarget()}},
                                       {QStringLiteral("anchor_id"), window->anchorId().isEmpty() ? QJsonValue {QJsonValue::Null} : QJsonValue {window->anchorId()}},
                                       {QStringLiteral("privacy_level"), QString::fromLatin1(astra::common::privacyLevelToString(window->privacyLevel()).data())},
                                       {QStringLiteral("focus_scope"), window->focusScope()}});
        }
        return {true, 0, {}, {{QStringLiteral("windows"), values}, {QStringLiteral("window_count"), windows_.openWindowCount()}}};
    }
    if (method == QStringLiteral("spatial_ui.focus.status")) {
        if (!params.isEmpty()) return {false, 5502, QStringLiteral("Focus query parameters must be empty"), {}};
        const auto record = focus_.record(QStringLiteral("workspace"));
        return {true, 0, {}, {{QStringLiteral("scope_id"), QStringLiteral("workspace")},
                              {QStringLiteral("focus_component_id"), record.componentId},
                              {QStringLiteral("focus_type"), focusTypeName(record.type)},
                              {QStringLiteral("priority"), static_cast<int>(record.priority)}, {QStringLiteral("reason"), record.reason}}};
    }
    if (method == QStringLiteral("spatial_ui.layout.status")) {
        return params.isEmpty() ? RuntimeResponse {true, 0, {}, {{QStringLiteral("mode"), layoutModeName(layoutMode_)}}}
                                : RuntimeResponse {false, 5302, QStringLiteral("Layout query parameters must be empty"), {}};
    }
    if (method == QStringLiteral("spatial_ui.metrics")) {
        if (!params.isEmpty()) return {false, 5103, QStringLiteral("Metrics query parameters must be empty"), {}};
        return {true, 0, {}, {{QStringLiteral("component_count"), components_.size()}, {QStringLiteral("window_count"), windows_.openWindowCount()},
                              {QStringLiteral("notification_count"), notifications_.size()}, {QStringLiteral("projection_safe"), projectionSafe_},
                              {QStringLiteral("projection_frame_id"), static_cast<qint64>(lastProjectionFrameId_)}}};
    }
    if (method == QStringLiteral("spatial_ui.component.create")) {
        const auto created = createComponent(params);
        if (!created.ok) return created;
        return synchronizeProjection(params.value(QStringLiteral("component_id")).toString(), traceId, requestId);
    }
    if (method == QStringLiteral("spatial_ui.task.create")) {
        auto created = createTaskSurface(params);
        if (!created.ok) return created;
        const auto synchronized = synchronizeProjection(params.value(QStringLiteral("task_id")).toString(), traceId, requestId);
        if (!synchronized.ok) return synchronized;
        for (auto iterator = synchronized.value.begin(); iterator != synchronized.value.end(); ++iterator) created.value.insert(iterator.key(), iterator.value());
        return created;
    }
    if (method == QStringLiteral("spatial_ui.task.update")) {
        auto updated = updateTaskSurface(params);
        if (!updated.ok) return updated;
        const auto synchronized = synchronizeProjection(params.value(QStringLiteral("task_id")).toString(), traceId, requestId);
        if (!synchronized.ok) return synchronized;
        for (auto iterator = synchronized.value.begin(); iterator != synchronized.value.end(); ++iterator) updated.value.insert(iterator.key(), iterator.value());
        return updated;
    }
    if (method == QStringLiteral("spatial_ui.component.update")) {
        auto updated = updateComponent(params);
        if (!updated.ok) return updated;
        const auto synchronized = synchronizeProjection(params.value(QStringLiteral("component_id")).toString(), traceId, requestId);
        if (!synchronized.ok) return synchronized;
        for (auto iterator = synchronized.value.begin(); iterator != synchronized.value.end(); ++iterator) {
            updated.value.insert(iterator.key(), iterator.value());
        }
        return updated;
    }
    if (method == QStringLiteral("spatial_ui.component.remove")) {
        // A notification component must go through the notification path so the
        // notification record and its critical-focus restore entry are released too.
        const QString removedId = params.value(QStringLiteral("component_id")).toString();
        const auto removed = exactKeys(params, {QStringLiteral("component_id")}) && notifications_.find(removedId)
            ? clearNotification({{QStringLiteral("notification_id"), removedId}})
            : removeComponent(params);
        if (!removed.ok) return removed;
        const auto synchronized = synchronizeProjection({}, traceId, requestId);
        return synchronized.ok ? removed : synchronized;
    }
    if (method == QStringLiteral("spatial_ui.window.open")) {
        auto opened = openWindow(params);
        if (!opened.ok) return opened;
        const auto synchronized = synchronizeProjection(opened.value.value(QStringLiteral("component_id")).toString(), traceId, requestId);
        if (!synchronized.ok) return synchronized;
        for (auto iterator = synchronized.value.begin(); iterator != synchronized.value.end(); ++iterator) {
            opened.value.insert(iterator.key(), iterator.value());
        }
        return opened;
    }
    if (method == QStringLiteral("spatial_ui.window.move") || method == QStringLiteral("spatial_ui.window.resize")
        || method == QStringLiteral("spatial_ui.window.hide") || method == QStringLiteral("spatial_ui.window.show")
        || method == QStringLiteral("spatial_ui.window.target") || method == QStringLiteral("spatial_ui.window.restore")
        || method == QStringLiteral("spatial_ui.window.close")) {
        auto updated = updateWindow(method, params);
        if (!updated.ok) return updated;
        const auto synchronized = synchronizeProjection(updated.value.value(QStringLiteral("component_id")).toString(), traceId, requestId);
        if (!synchronized.ok) return synchronized;
        for (auto iterator = synchronized.value.begin(); iterator != synchronized.value.end(); ++iterator) {
            updated.value.insert(iterator.key(), iterator.value());
        }
        return updated;
    }
    if (method == QStringLiteral("spatial_ui.focus")) {
        if (!exactKeys(params, {QStringLiteral("component_id"), QStringLiteral("reason")})
            || !isUuid(params.value(QStringLiteral("component_id")).toString())
            || !params.value(QStringLiteral("reason")).isString()
            || params.value(QStringLiteral("reason")).toString().isEmpty()) {
            return {false, 5502, QStringLiteral("Invalid focus parameters"), {}};
        }
        const QString previousFocus = focus_.owner(QStringLiteral("workspace"));
        const QString reason = params.value(QStringLiteral("reason")).toString();
        const auto result = focus_.requestFocus(params.value(QStringLiteral("component_id")).toString(), astra::ui::FocusType::Keyboard,
                                                astra::ui::FocusPriority::ActiveWindow, params.value(QStringLiteral("reason")).toString());
        return {result.ok, result.errorCode, result.message,
                {{QStringLiteral("focus_component_id"), focus_.owner(QStringLiteral("workspace"))},
                 {QStringLiteral("previous_focus_component_id"), previousFocus}, {QStringLiteral("reason"), reason}}};
    }
    if (method == QStringLiteral("spatial_ui.input")) {
        const QSet<QString> requiredKeys {QStringLiteral("schema_version"), QStringLiteral("event_id"), QStringLiteral("event_type"),
                                          QStringLiteral("source_type"), QStringLiteral("source_id"), QStringLiteral("target_component_id"),
                                          QStringLiteral("position"), QStringLiteral("modifiers"), QStringLiteral("timestamp")};
        QSet<QString> suppliedKeys;
        for (auto iterator = params.begin(); iterator != params.end(); ++iterator) suppliedKeys.insert(iterator.key());
        const QSet<QString> allowedKeys = requiredKeys | QSet<QString> {QStringLiteral("interaction_value"), QStringLiteral("action")};
        const auto eventType = inputEventType(params.value(QStringLiteral("event_type")).toString());
        const auto sourceType = inputSourceType(params.value(QStringLiteral("source_type")).toString());
        const auto timestamp = QDateTime::fromString(params.value(QStringLiteral("timestamp")).toString(), Qt::ISODateWithMs);
        if (!(requiredKeys - suppliedKeys).isEmpty() || !(suppliedKeys - allowedKeys).isEmpty()
            || params.value(QStringLiteral("schema_version")).toString() != QStringLiteral("1.0")
            || QUuid {params.value(QStringLiteral("event_id")).toString()}.isNull() || !eventType || !sourceType
            || params.value(QStringLiteral("source_id")).toString().isEmpty()
            || (!params.value(QStringLiteral("target_component_id")).isNull()
                && (!params.value(QStringLiteral("target_component_id")).isString()
                    || !isUuid(params.value(QStringLiteral("target_component_id")).toString())))
            || !params.value(QStringLiteral("modifiers")).isArray() || !timestamp.isValid() || timestamp.offsetFromUtc() != 0) {
            return {false, 5402, QStringLiteral("Invalid input event"), {}};
        }
        const bool sourceEnabled = (*sourceType == astra::ui::InputSourceType::Mouse && options_.mouseEnabled)
            || (*sourceType == astra::ui::InputSourceType::Keyboard && options_.keyboardEnabled)
            || (*sourceType == astra::ui::InputSourceType::Touch && options_.touchEnabled)
            || (*sourceType == astra::ui::InputSourceType::SimulatedGesture && options_.simulatedGestureEnabled)
            || *sourceType == astra::ui::InputSourceType::System || *sourceType == astra::ui::InputSourceType::AiIntent;
        if (!sourceEnabled) return {false, 5401, QStringLiteral("Input source is disabled by validated configuration"), {}};
        static const QSet<QString> allowedModifiers {QStringLiteral("SHIFT"), QStringLiteral("CONTROL"),
                                                     QStringLiteral("ALT"), QStringLiteral("META")};
        QSet<QString> modifiers;
        for (const auto &modifier : params.value(QStringLiteral("modifiers")).toArray()) {
            if (!modifier.isString() || !allowedModifiers.contains(modifier.toString()) || modifiers.contains(modifier.toString())) {
                return {false, 5402, QStringLiteral("Invalid input modifiers"), {}};
            }
            modifiers.insert(modifier.toString());
        }
        const bool transformGesture = *eventType == astra::ui::InputEventType::GestureScale
            || *eventType == astra::ui::InputEventType::GestureRotate;
        const bool actionEvent = *eventType == astra::ui::InputEventType::AiAction;
        if (actionEvent != params.contains(QStringLiteral("action"))
            || (actionEvent && (params.value(QStringLiteral("action")).toString().isEmpty()
                                || params.value(QStringLiteral("action")).toString().size() > 128
                                || (*sourceType != astra::ui::InputSourceType::System
                                    && *sourceType != astra::ui::InputSourceType::AiIntent)))) {
            return {false, 5402, QStringLiteral("Input action does not match event type"), {}};
        }
        std::optional<double> interactionValue;
        if (params.contains(QStringLiteral("interaction_value")) && !params.value(QStringLiteral("interaction_value")).isNull()) {
            if (!params.value(QStringLiteral("interaction_value")).isDouble()) {
                return {false, 5402, QStringLiteral("Invalid interaction value"), {}};
            }
            interactionValue = params.value(QStringLiteral("interaction_value")).toDouble();
        }
        if (transformGesture != interactionValue.has_value() || (interactionValue && !std::isfinite(*interactionValue))
            || (*eventType == astra::ui::InputEventType::GestureScale
                && (*interactionValue < 0.1 || *interactionValue > 10.0))
            || (*eventType == astra::ui::InputEventType::GestureRotate
                && (*interactionValue < -360.0 || *interactionValue > 360.0))) {
            return {false, 5402, QStringLiteral("Interaction value does not match event type"), {}};
        }
        astra::ui::CoordinateSystem coordinates = astra::ui::CoordinateSystem::PhoneView;
        double x = 0.0;
        double y = 0.0;
        if (params.value(QStringLiteral("position")).isObject()) {
            const auto position = params.value(QStringLiteral("position")).toObject();
            const auto parsedCoordinates = coordinateSystem(position.value(QStringLiteral("coordinate_system")).toString());
            if (!exactKeys(position, {QStringLiteral("coordinate_system"), QStringLiteral("x"), QStringLiteral("y")})
                || !parsedCoordinates || !position.value(QStringLiteral("x")).isDouble() || !position.value(QStringLiteral("y")).isDouble()) {
                return {false, 5402, QStringLiteral("Invalid input position"), {}};
            }
            coordinates = *parsedCoordinates;
            x = position.value(QStringLiteral("x")).toDouble();
            y = position.value(QStringLiteral("y")).toDouble();
        } else if (!params.value(QStringLiteral("position")).isNull()
                   || (*eventType != astra::ui::InputEventType::KeyPress && *eventType != astra::ui::InputEventType::KeyRelease
                       && *eventType != astra::ui::InputEventType::SystemFocus && *eventType != astra::ui::InputEventType::AiAction)) {
            return {false, 5402, QStringLiteral("Input position is required for positional events"), {}};
        }
        QList<astra::ui::HitTarget> targets;
        targets.reserve(components_.size());
        for (const auto *component : components_.components()) {
            int hierarchyDepth = 0;
            QSet<QString> ancestors;
            QString ancestorId = component->parentId();
            while (!ancestorId.isEmpty() && !ancestors.contains(ancestorId)) {
                ancestors.insert(ancestorId);
                const auto *ancestor = components_.find(ancestorId);
                if (!ancestor) break;
                ++hierarchyDepth;
                ancestorId = ancestor->parentId();
            }
            targets.append({component->componentId(), component->bounds(), astra::ui::HitShape::Rectangle, {}, component->zOrder(),
                            isSecurityLayer(component->componentType())
                                || (notifications_.find(component->componentId())
                                    && notifications_.find(component->componentId())->requestsSystemFocus()),
                            false, component->isVisible(), component->isInteractive(),
                            component->opacity(), component->isEnabled(), component->parentId(), hierarchyDepth});
        }
        input_.setTargets(std::move(targets));
        astra::ui::InputEvent event {params.value(QStringLiteral("event_id")).toString(), *eventType, *sourceType,
                                     params.value(QStringLiteral("source_id")).toString(), coordinates, x, y,
                                     params.value(QStringLiteral("target_component_id")).toString(),
                                     interactionValue};
        const QString sourceKey = interactionSourceKey(event);
        if (interactionTargetsBySource_.contains(sourceKey)) {
            event.targetComponentId = interactionTargetsBySource_.value(sourceKey);
            event.targetCaptured = true;
        }
        const auto routed = input_.route(event);
        if (!routed.ok) return {routed.ok, routed.errorCode, routed.message,
                                {{QStringLiteral("target_component_id"), routed.targetComponentId}, {QStringLiteral("blocked"), routed.blocked}}};
        if (*eventType == astra::ui::InputEventType::SystemFocus) {
            const auto *component = components_.find(routed.targetComponentId);
            if (!component) return {false, 5501, QStringLiteral("Focus target is unavailable"), {}};
            astra::ui::FocusType focusType = astra::ui::FocusType::Pointer;
            if (*sourceType == astra::ui::InputSourceType::Keyboard) focusType = astra::ui::FocusType::Keyboard;
            else if (*sourceType == astra::ui::InputSourceType::Touch) focusType = astra::ui::FocusType::Touch;
            else if (*sourceType == astra::ui::InputSourceType::SimulatedGesture) focusType = astra::ui::FocusType::GestureSimulated;
            else if (*sourceType == astra::ui::InputSourceType::System) focusType = astra::ui::FocusType::System;
            const auto priority = *sourceType == astra::ui::InputSourceType::System
                ? astra::ui::FocusPriority::System : astra::ui::FocusPriority::ActiveWindow;
            const QString previousFocus = focus_.owner(QStringLiteral("workspace"));
            const auto focused = focus_.requestFocus(routed.targetComponentId, focusType, priority,
                                                     QStringLiteral("routed unified input"));
            return {focused.ok, focused.errorCode, focused.message,
                    {{QStringLiteral("target_component_id"), routed.targetComponentId},
                     {QStringLiteral("previous_focus_component_id"), previousFocus},
                     {QStringLiteral("focus_component_id"), focus_.owner(QStringLiteral("workspace"))},
                     {QStringLiteral("blocked"), false}, {QStringLiteral("interaction_state"), QStringLiteral("IDLE")},
                     {QStringLiteral("interaction_completed"), focused.ok}}};
        }
        const auto *notification = notifications_.find(routed.targetComponentId);
        if (*eventType == astra::ui::InputEventType::AiAction) {
            const QString action = params.value(QStringLiteral("action")).toString();
            if (notification && notification->spec().requiresAction) {
                if (!notification->spec().actions.contains(action)) {
                    return {false, 5402, QStringLiteral("Notification action is not declared"),
                            {{QStringLiteral("target_component_id"), routed.targetComponentId}, {QStringLiteral("blocked"), false}}};
                }
                const auto cleared = clearNotification({{QStringLiteral("notification_id"), routed.targetComponentId}});
                if (!cleared.ok) return cleared;
                const auto synchronized = synchronizeProjection(routed.targetComponentId, traceId, requestId);
                if (!synchronized.ok) return synchronized;
                QJsonObject value = cleared.value;
                value.insert(QStringLiteral("target_component_id"), routed.targetComponentId);
                value.insert(QStringLiteral("blocked"), false);
                value.insert(QStringLiteral("notification_cleared"), true);
                value.insert(QStringLiteral("interaction_state"), QStringLiteral("IDLE"));
                value.insert(QStringLiteral("interaction_completed"), true);
                return {true, 0, {}, value};
            }
            auto *task = tasks_.find(routed.targetComponentId);
            if (!task) return {false, 5402, QStringLiteral("Action target does not accept system actions"), {}};
            astra::ui::OperationResult taskAction;
            if (action == QStringLiteral("confirm") && task->state() == astra::ui::TaskSurfaceState::WaitingConfirmation) {
                taskAction = task->transitionTo(astra::ui::TaskSurfaceState::Ready);
            } else if (action == QStringLiteral("cancel")) {
                taskAction = task->transitionTo(astra::ui::TaskSurfaceState::Cancelled);
            } else {
                return {false, 5402, QStringLiteral("Task action is not available in the current state"), {}};
            }
            if (!taskAction.ok) return {false, taskAction.errorCode, taskAction.message, {}};
            const auto synchronized = synchronizeProjection(routed.targetComponentId, traceId, requestId);
            if (!synchronized.ok) return synchronized;
            return {true, 0, {}, {{QStringLiteral("target_component_id"), routed.targetComponentId},
                                  {QStringLiteral("task_state"), taskSurfaceStateName(task->state())},
                                  {QStringLiteral("blocked"), false},
                                  {QStringLiteral("interaction_state"), QStringLiteral("IDLE")},
                                  {QStringLiteral("interaction_completed"), true}}};
        }
        auto interaction = applyInputInteraction(event, routed.targetComponentId);
        if (!interaction.ok) return interaction;
        interaction.value.insert(QStringLiteral("target_component_id"), routed.targetComponentId);
        interaction.value.insert(QStringLiteral("blocked"), routed.blocked);
        return interaction;
    }
    if (method == QStringLiteral("spatial_ui.interaction.cancel")) return cancelInteraction(params);
    if (method == QStringLiteral("spatial_ui.layout.apply")) {
        auto applied = applyLayout(params);
        if (!applied.ok) return applied;
        const auto synchronized = synchronizeProjection({}, traceId, requestId);
        if (!synchronized.ok) return synchronized;
        for (auto iterator = synchronized.value.begin(); iterator != synchronized.value.end(); ++iterator) {
            applied.value.insert(iterator.key(), iterator.value());
        }
        return applied;
    }
    if (method == QStringLiteral("spatial_ui.layout.reset")) {
        if (!params.isEmpty()) return {false, 5302, QStringLiteral("Layout reset parameters must be empty"), {}};
        QStringList ids;
        for (const auto *component : components_.components()) ids.append(component->componentId());
        std::sort(ids.begin(), ids.end());
        QJsonArray componentIds;
        for (const auto &id : ids) componentIds.append(id);
        auto applied = applyLayout({{QStringLiteral("mode"), layoutModeName(options_.defaultLayout)},
                                    {QStringLiteral("direction"), QStringLiteral("VERTICAL")},
                                    {QStringLiteral("component_ids"), componentIds},
                                    {QStringLiteral("safe_area"), QJsonObject {{QStringLiteral("x"), 0.0}, {QStringLiteral("y"), 0.0},
                                                                             {QStringLiteral("width"), 1280.0}, {QStringLiteral("height"), 720.0}}},
                                    {QStringLiteral("spacing"), options_.layoutSpacing}, {QStringLiteral("margin"), options_.layoutMargin},
                                    {QStringLiteral("columns"), 0}, {QStringLiteral("radial_radius"), 120.0},
                                    {QStringLiteral("anchor"), QJsonValue {QJsonValue::Null}}});
        if (!applied.ok) return applied;
        const auto synchronized = synchronizeProjection({}, traceId, requestId);
        if (!synchronized.ok) return synchronized;
        for (auto iterator = synchronized.value.begin(); iterator != synchronized.value.end(); ++iterator) {
            applied.value.insert(iterator.key(), iterator.value());
        }
        return applied;
    }
    if (method == QStringLiteral("spatial_ui.notification.create") || method == QStringLiteral("spatial_ui.notification.clear")) {
        auto changed = method.endsWith(QStringLiteral("create")) ? createNotification(params) : clearNotification(params);
        if (!changed.ok) return changed;
        const auto synchronized = synchronizeProjection(changed.value.value(QStringLiteral("notification_id")).toString(), traceId, requestId);
        if (!synchronized.ok) return synchronized;
        for (auto iterator = synchronized.value.begin(); iterator != synchronized.value.end(); ++iterator) {
            changed.value.insert(iterator.key(), iterator.value());
        }
        return changed;
    }
    if (method == QStringLiteral("spatial_ui.reset")) {
        if (!params.isEmpty()) return {false, 5103, QStringLiteral("Reset parameters must be empty"), {}};
        const auto cleared = projectionGateway_->clear(QStringLiteral("MANUAL_STOP"), traceId, requestId);
        if (!cleared.ok) return {false, 5904, cleared.message, {}};
        input_.setTargets({});
        focus_.clear();
        notifications_.clear();
        windows_.clear();
        components_.clear();
        criticalFocusRestoreChain_.clear();
        interactionSessions_.clear();
        interactionStartPositions_.clear();
        interactionTargetsBySource_.clear();
        tasks_.clear();
        layoutMode_ = options_.defaultLayout;
        projectionSafe_ = true;
        projectionContentActive_ = false;
        lastProjectionFrameId_ = 0;
        return {true, 0, {}, {{QStringLiteral("projection_safe"), true}, {QStringLiteral("component_count"), 0},
                              {QStringLiteral("window_count"), 0}, {QStringLiteral("notification_count"), 0}}};
    }
    if (method == QStringLiteral("spatial_ui.state.save")) {
        if (!params.isEmpty()) return {false, 5803, QStringLiteral("State save parameters must be empty"), {}};
        return saveState();
    }
    if (method == QStringLiteral("spatial_ui.state.load")) {
        if (!params.isEmpty()) return {false, 5803, QStringLiteral("State load parameters must be empty"), {}};
        if (!options_.statePersistence) return {false, 5802, QStringLiteral("State persistence is disabled"), {}};
        const auto result = stateStore_.load();
        if (!result.ok) return {false, result.errorCode, result.message, {}};
        if (components_.size() != 0 || windows_.size() != 0) return {false, 5803, QStringLiteral("State restore requires an empty runtime"), {}};
        // Restore into the empty runtime atomically: a partial restore would leave
        // orphaned state and make every later load fail the empty-runtime check.
        const auto rollback = [this](RuntimeResponse failed) {
            input_.setTargets({});
            focus_.clear();
            windows_.clear();
            components_.clear();
            return failed;
        };
        for (const auto &saved : result.snapshot.components) {
            astra::ui::ComponentSpec spec;
            spec.id = saved.componentId;
            spec.type = saved.componentType;
            spec.bounds = saved.bounds;
            spec.zOrder = saved.zOrder;
            spec.focusable = saved.focusable;
            spec.interactive = saved.interactive;
            spec.privacyLevel = classifyFixture(saved.componentId, options_.maximumFixtureDisclosure).privacyLevel;
            spec.displayTarget = saved.displayTarget;
            spec.accessibilityLabel = QStringLiteral("Restored component %1").arg(saved.componentId.left(8));
            const auto created = components_.create(spec, astra::ui::Principal::Application);
            if (!created.ok || !created.component->transitionTo(astra::ui::ComponentLifecycleState::Attached).ok
                || !created.component->transitionTo(astra::ui::ComponentLifecycleState::Visible).ok) {
                return rollback({false, 5803, QStringLiteral("Component state restore failed"), {}});
            }
            if (!saved.visible && !created.component->transitionTo(astra::ui::ComponentLifecycleState::Hidden).ok) {
                return rollback({false, 5803, QStringLiteral("Component visibility restore failed"), {}});
            }
            if (!focus_.registerTarget({saved.componentId, QStringLiteral("workspace"), saved.visible, saved.interactive,
                                        saved.focusable, saved.zOrder}).ok) {
                return rollback({false, 5803, QStringLiteral("Component focus restore failed"), {}});
            }
        }
        for (const auto &saved : result.snapshot.windows) {
            const auto *component = components_.find(saved.componentId);
            if (!component) return rollback({false, 5805, QStringLiteral("Window component state is missing"), {}});
            astra::ui::SpatialWindowSpec spec;
            spec.windowId = saved.windowId;
            spec.componentId = saved.componentId;
            spec.bounds = saved.bounds;
            spec.displayTarget = saved.displayTarget;
            spec.projectionTarget = saved.projectionTarget;
            spec.anchorId = saved.anchorId;
            spec.privacyLevel = component->privacyLevel();
            spec.focusScope = saved.focusScope;
            if (!windows_.createWindow(spec).ok || !windows_.showWindow(saved.windowId).ok
                || (!saved.visible && !windows_.hideWindow(saved.windowId).ok)) {
                return rollback({false, 5805, QStringLiteral("Window state restore failed"), {}});
            }
        }
        if (!result.snapshot.focusRestoreComponentId.isEmpty()
            && !focus_.requestFocus(result.snapshot.focusRestoreComponentId, astra::ui::FocusType::Keyboard,
                                    astra::ui::FocusPriority::ActiveWindow, QStringLiteral("state restore")).ok) {
            return rollback({false, 5504, QStringLiteral("Focus state restore failed"), {}});
        }
        layoutMode_ = result.snapshot.layoutMode;
        selectedTab_ = result.snapshot.selectedTab;
        panelOrder_ = result.snapshot.panelOrder;
        const auto synchronized = synchronizeProjection({}, traceId, requestId);
        if (!synchronized.ok) return synchronized;
        return {true, 0, {}, {{QStringLiteral("selected_tab"), result.snapshot.selectedTab},
                              {QStringLiteral("component_count"), components_.size()}, {QStringLiteral("window_count"), windows_.openWindowCount()},
                              {QStringLiteral("focus_component_id"), focus_.owner(QStringLiteral("workspace"))}}};
    }
    if (method == QStringLiteral("spatial_ui.target.lost")) {
        if (!params.isEmpty()) return {false, 5206, QStringLiteral("Target loss parameters must be empty"), {}};
        targetAvailable_ = false;
        for (auto *component : components_.components()) {
            if (component->isVisible()) static_cast<void>(component->transitionTo(astra::ui::ComponentLifecycleState::Hidden));
            static_cast<void>(focus_.setAvailable(component->componentId(), false));
        }
        const auto cleared = projectionGateway_->clear(QStringLiteral("TARGET_LOST"), traceId, requestId);
        projectionSafe_ = true;
        projectionContentActive_ = false;
        lastProjectionFrameId_ = 0;
        return {cleared.ok, cleared.ok ? 0 : 5904, cleared.message, {{QStringLiteral("projection_safe"), true}}};
    }
    if (method == QStringLiteral("spatial_ui.target.available")) {
        if (!params.isEmpty()) return {false, 5206, QStringLiteral("Target availability parameters must be empty"), {}};
        targetAvailable_ = true;
        projectionSafe_ = true;
        return {true, 0, {}, {{QStringLiteral("target_available"), true}, {QStringLiteral("projection_safe"), true}}};
    }
    return {false, 5905, QStringLiteral("Unknown spatial UI method"), {}};
}

QJsonObject SpatialUIRuntime::status() const
{
    return {{QStringLiteral("status"), QStringLiteral("READY")}, {QStringLiteral("component_count"), components_.size()},
            {QStringLiteral("window_count"), windows_.openWindowCount()}, {QStringLiteral("notification_count"), notifications_.size()},
            {QStringLiteral("focus_component_id"), focus_.owner(QStringLiteral("workspace"))},
            {QStringLiteral("projection_safe"), projectionSafe_}, {QStringLiteral("projection_frame_id"), static_cast<qint64>(lastProjectionFrameId_)},
            {QStringLiteral("configuration_status"), options_.configurationStatus},
            {QStringLiteral("configuration_warning"), options_.configurationWarning},
            {QStringLiteral("state_persistence_enabled"), options_.statePersistence},
            {QStringLiteral("reduce_motion"), options_.reduceMotion},
            {QStringLiteral("high_contrast"), options_.highContrast},
            {QStringLiteral("p4_release_status"), QStringLiteral("P4_RELEASE_BASELINE_FINAL")}};
}

RuntimeResponse SpatialUIRuntime::createComponent(const QJsonObject &params)
{
    const QSet<QString> requiredKeys {QStringLiteral("component_id"), QStringLiteral("component_type"), QStringLiteral("privacy_level"),
                                      QStringLiteral("accessibility_label"), QStringLiteral("bounds"), QStringLiteral("display_target")};
    const QSet<QString> allowedKeys = requiredKeys | QSet<QString> {QStringLiteral("parent_id"), QStringLiteral("children")};
    QSet<QString> suppliedKeys;
    for (auto iterator = params.begin(); iterator != params.end(); ++iterator) suppliedKeys.insert(iterator.key());
    if (!(requiredKeys - suppliedKeys).isEmpty() || !(suppliedKeys - allowedKeys).isEmpty()) {
        return {false, 5103, QStringLiteral("Invalid component parameters"), {}};
    }
    const auto type = componentType(params.value(QStringLiteral("component_type")).toString());
    const auto privacy = astra::common::privacyLevelFromString(params.value(QStringLiteral("privacy_level")).toString().toStdString());
    const auto target = displayTarget(params.value(QStringLiteral("display_target")).toString());
    const auto componentBounds = bounds(params.value(QStringLiteral("bounds")).toObject());
    const QString componentId = params.value(QStringLiteral("component_id")).toString();
    const QString parentId = params.value(QStringLiteral("parent_id")).isString()
        ? params.value(QStringLiteral("parent_id")).toString() : QString {};
    if (!type || !privacy || !target || !componentBounds || QUuid {componentId}.isNull()
        || (params.contains(QStringLiteral("parent_id")) && !params.value(QStringLiteral("parent_id")).isNull()
            && (!params.value(QStringLiteral("parent_id")).isString() || !isUuid(parentId)))
        || (params.contains(QStringLiteral("children")) && !params.value(QStringLiteral("children")).isArray())) {
        return {false, 5103, QStringLiteral("Invalid component domain value"), {}};
    }
    QStringList children;
    for (const auto &childValue : params.value(QStringLiteral("children")).toArray()) {
        const QString childId = childValue.toString();
        if (!childValue.isString() || !isUuid(childId) || children.contains(childId)) {
            return {false, 5103, QStringLiteral("Invalid component hierarchy"), {}};
        }
        children.append(childId);
    }
    const auto classification = classifyFixture(componentId, *privacy);
    astra::ui::ComponentSpec spec;
    spec.id = componentId;
    spec.type = *type;
    spec.parentId = parentId;
    spec.children = children;
    spec.bounds = *componentBounds;
    spec.zOrder = static_cast<int>(components_.size());
    spec.focusable = true;
    spec.interactive = true;
    spec.privacyLevel = classification.privacyLevel;
    spec.displayTarget = *target;
    spec.accessibilityLabel = params.value(QStringLiteral("accessibility_label")).toString();
    auto created = components_.create(spec, astra::ui::Principal::Application);
    if (!created.ok) return {false, created.errorCode, created.message, {}};
    static_cast<void>(created.component->transitionTo(astra::ui::ComponentLifecycleState::Attached));
    static_cast<void>(created.component->transitionTo(astra::ui::ComponentLifecycleState::Visible));
    static_cast<void>(focus_.registerTarget({spec.id, QStringLiteral("workspace"), true, true, true, spec.zOrder}));
    return {true, 0, {}, {{QStringLiteral("component_id"), spec.id}, {QStringLiteral("parent_id"), parentId},
                          {QStringLiteral("children"), QJsonArray::fromStringList(children)},
                          {QStringLiteral("projection_layer_generated"), false}}};
}

RuntimeResponse SpatialUIRuntime::createTaskSurface(const QJsonObject &params)
{
    const QSet<QString> keys {QStringLiteral("schema_version"), QStringLiteral("task_id"), QStringLiteral("title"),
                              QStringLiteral("summary"), QStringLiteral("intent_type"), QStringLiteral("confidence"),
                              QStringLiteral("execution_strategy"), QStringLiteral("privacy_level"), QStringLiteral("bounds"),
                              QStringLiteral("display_target"), QStringLiteral("accessibility_label")};
    const auto privacy = astra::common::privacyLevelFromString(params.value(QStringLiteral("privacy_level")).toString().toStdString());
    const auto target = displayTarget(params.value(QStringLiteral("display_target")).toString());
    const auto componentBounds = bounds(params.value(QStringLiteral("bounds")).toObject());
    const double confidence = params.value(QStringLiteral("confidence")).toDouble(-1.0);
    if (!exactKeys(params, keys) || params.value(QStringLiteral("schema_version")).toString() != QStringLiteral("1.0")
        || !isUuid(params.value(QStringLiteral("task_id")).toString()) || params.value(QStringLiteral("title")).toString().isEmpty()
        || params.value(QStringLiteral("summary")).toString().isEmpty() || params.value(QStringLiteral("intent_type")).toString().isEmpty()
        || !std::isfinite(confidence) || confidence < 0.0 || confidence > 1.0
        || params.value(QStringLiteral("execution_strategy")).toString().isEmpty() || !privacy || !target || !componentBounds
        || params.value(QStringLiteral("accessibility_label")).toString().isEmpty()) {
        return {false, 5103, QStringLiteral("Invalid task fixture parameters"), {}};
    }
    const QString taskId = params.value(QStringLiteral("task_id")).toString();
    const auto classification = classifyFixture(taskId, *privacy);
    QJsonObject fixture = params;
    fixture.insert(QStringLiteral("privacy_level"), QString::fromLatin1(astra::common::privacyLevelToString(classification.privacyLevel).data()));
    const auto task = tasks_.createFromIntentFixture(fixture);
    if (!task.ok) return {false, task.errorCode, task.message, {}};
    if (task.surface->executionStrategy() == QStringLiteral("REQUIRE_CONFIRMATION")) {
        const auto waiting = task.surface->transitionTo(astra::ui::TaskSurfaceState::WaitingConfirmation);
        if (!waiting.ok) {
            static_cast<void>(tasks_.remove(taskId));
            return {false, waiting.errorCode, waiting.message, {}};
        }
    }
    astra::ui::ComponentSpec spec;
    spec.id = taskId;
    spec.type = astra::ui::ComponentType::TaskCard;
    spec.bounds = *componentBounds;
    spec.zOrder = static_cast<int>(components_.size());
    spec.focusable = true;
    spec.interactive = true;
    spec.privacyLevel = classification.privacyLevel;
    spec.displayTarget = *target;
    spec.accessibilityLabel = params.value(QStringLiteral("accessibility_label")).toString();
    const auto component = components_.create(spec, astra::ui::Principal::Application);
    if (!component.ok) {
        static_cast<void>(tasks_.remove(taskId));
        return {false, component.errorCode, component.message, {}};
    }
    static_cast<void>(component.component->transitionTo(astra::ui::ComponentLifecycleState::Attached));
    static_cast<void>(component.component->transitionTo(astra::ui::ComponentLifecycleState::Visible));
    static_cast<void>(focus_.registerTarget({taskId, QStringLiteral("workspace"), true, true, true, spec.zOrder}));
    return {true, 0, {}, {{QStringLiteral("task_id"), taskId}, {QStringLiteral("component_id"), taskId},
                          {QStringLiteral("task_surface_created"), true}, {QStringLiteral("task_surface_updated"), false},
                          {QStringLiteral("privacy_level"), QString::fromLatin1(astra::common::privacyLevelToString(spec.privacyLevel).data())}}};
}

RuntimeResponse SpatialUIRuntime::updateTaskSurface(const QJsonObject &params)
{
    const QSet<QString> keys {QStringLiteral("schema_version"), QStringLiteral("task_id"), QStringLiteral("state"), QStringLiteral("progress")};
    if (!exactKeys(params, keys) || params.value(QStringLiteral("schema_version")).toString() != QStringLiteral("1.0")
        || !isUuid(params.value(QStringLiteral("task_id")).toString())
        || (!params.value(QStringLiteral("progress")).isDouble() && !params.value(QStringLiteral("progress")).isUndefined())
        || (!params.value(QStringLiteral("state")).isString() && !params.value(QStringLiteral("state")).isUndefined())) {
        return {false, 5103, QStringLiteral("Invalid task update parameters"), {}};
    }
    const auto updated = tasks_.updateFromIntentFixture(params);
    if (!updated.ok) return {false, updated.errorCode, updated.message, {}};
    auto *component = components_.find(params.value(QStringLiteral("task_id")).toString());
    if (!component) return {false, 5105, QStringLiteral("Task component not found"), {}};
    return {true, 0, {}, {{QStringLiteral("task_id"), updated.surface->taskId()}, {QStringLiteral("component_id"), component->componentId()},
                          {QStringLiteral("task_surface_created"), false}, {QStringLiteral("task_surface_updated"), true},
                          {QStringLiteral("progress"), updated.surface->progress()}}};
}

QVector<astra::render::ProjectionLayer> SpatialUIRuntime::projectionLayers() const
{
    QVector<astra::render::ProjectionLayer> layers;
    for (const auto *component : components_.components()) {
        if (component->displayTarget() == astra::ui::DisplayTarget::Phone) continue;
        astra::policy::ProjectionPolicyRequest request {component->privacyLevel(), options_.authorizedPersonPresent};
        request.roomTrusted = options_.roomTrusted;
        request.projectionTargetEnabled = options_.projectionTargetEnabled;
        request.targetAvailable = targetAvailable_;
        request.currentState = projectionContentActive_ ? QStringLiteral("ACTIVE") : QStringLiteral("IDLE");
        request.subjectId = component->componentId();
        const auto policyDecision = astra::policy::ProjectionPolicyService::evaluate(request, options_.policyBindingKey);
        const auto layer = layerMapper_.map(*component, policyDecision);
        if (layer) layers.append(*layer);
    }
    return astra::render::compositionOrder(layers);
}

RuntimeResponse SpatialUIRuntime::synchronizeProjection(const QString &componentId, const QString &traceId, const QString &requestId)
{
    const auto layers = projectionLayers();
    const bool generated = std::any_of(layers.cbegin(), layers.cend(), [&componentId](const auto &layer) { return layer.id == componentId; });
    if (layers.isEmpty()) {
        if (projectionContentActive_) {
            const auto cleared = projectionGateway_->clear(QStringLiteral("NO_PROJECTION"), traceId, requestId);
            if (!cleared.ok) return {false, 5904, cleared.message, {}};
        }
        projectionSafe_ = true;
        projectionContentActive_ = false;
        lastProjectionFrameId_ = 0;
        QJsonObject value {{QStringLiteral("projection_layer_generated"), generated}};
        if (!componentId.isEmpty()) value.insert(QStringLiteral("component_id"), componentId);
        return {true, 0, {}, value};
    }
    const auto submitted = projectionGateway_->submit(layers, traceId, requestId);
    if (!submitted.ok) {
        static_cast<void>(projectionGateway_->clear(QStringLiteral("SERVICE_EXCEPTION"), traceId, requestId));
        targetAvailable_ = false;
        projectionSafe_ = true;
        projectionContentActive_ = false;
        lastProjectionFrameId_ = 0;
        return {false, 5904, submitted.message, {}};
    }
    projectionSafe_ = false;
    projectionContentActive_ = true;
    lastProjectionFrameId_ = submitted.frameId;
    QJsonObject value {{QStringLiteral("projection_layer_generated"), generated},
                       {QStringLiteral("projection_frame_id"), static_cast<qint64>(submitted.frameId)},
                       {QStringLiteral("submitted_layer_count"), layers.size()}};
    if (!componentId.isEmpty()) value.insert(QStringLiteral("component_id"), componentId);
    return {true, 0, {}, value};
}

RuntimeResponse SpatialUIRuntime::applyInputInteraction(const astra::ui::InputEvent &event, const QString &targetComponentId)
{
    if (targetComponentId.isEmpty()) return {true, 0, {}, {{QStringLiteral("interaction_state"), QStringLiteral("IDLE")}}};
    auto *component = components_.find(targetComponentId);
    if (!component) return {false, 5105, QStringLiteral("Interaction component not found"), {}};
    const auto eventType = event.eventType;
    if (eventType == astra::ui::InputEventType::PointerPress || eventType == astra::ui::InputEventType::GestureGrab
        || eventType == astra::ui::InputEventType::TouchBegin) {
        interactionSessions_.remove(targetComponentId);
        auto session = std::make_shared<astra::ui::InteractionSession>(*component, astra::ui::SpatialBounds {0.0, 0.0, 1280.0, 720.0});
        auto pressed = session->press();
        if (!pressed.ok) return {false, pressed.errorCode, pressed.message, {}};
        const astra::ui::InteractionState mode = astra::ui::InteractionState::Dragging;
        if (eventType != astra::ui::InputEventType::PointerPress) {
            const auto began = session->begin(mode);
            if (!began.ok) return {false, began.errorCode, began.message, {}};
        }
        interactionStartPositions_.insert(targetComponentId, QPointF {event.x, event.y});
        interactionTargetsBySource_.insert(interactionSourceKey(event), targetComponentId);
        const auto state = session->machine().state();
        interactionSessions_.insert(targetComponentId, std::move(session));
        return {true, 0, {}, {{QStringLiteral("interaction_state"), interactionStateName(state)},
                              {QStringLiteral("interaction_started"), true}}};
    }
    auto iterator = interactionSessions_.find(targetComponentId);
    if (iterator == interactionSessions_.end()) return {true, 0, {}, {{QStringLiteral("interaction_state"), QStringLiteral("IDLE")}}};
    auto &session = *iterator.value();
    const QPointF start = interactionStartPositions_.value(targetComponentId, QPointF {event.x, event.y});
    astra::ui::OperationResult operation;
    if (eventType == astra::ui::InputEventType::GestureScale || eventType == astra::ui::InputEventType::GestureRotate) {
        operation = session.cancel();
        if (!operation.ok) return {false, operation.errorCode, operation.message, {}};
        operation = synchronizeWindowBounds(targetComponentId);
        if (!operation.ok) return {false, operation.errorCode, operation.message, {}};
        interactionSessions_.remove(targetComponentId);
        interactionStartPositions_.remove(targetComponentId);
        auto transformed = std::make_shared<astra::ui::InteractionSession>(*component, astra::ui::SpatialBounds {0.0, 0.0, 1280.0, 720.0});
        operation = transformed->press();
        if (operation.ok) operation = transformed->begin(eventType == astra::ui::InputEventType::GestureScale
                                                             ? astra::ui::InteractionState::Scaling
                                                             : astra::ui::InteractionState::Rotating);
        if (operation.ok) operation = eventType == astra::ui::InputEventType::GestureScale
            ? transformed->scaleBy(*event.interactionValue) : transformed->rotateBy(*event.interactionValue);
        if (!operation.ok) return {false, operation.errorCode, operation.message, {}};
        const QString state = interactionStateName(transformed->machine().state());
        interactionSessions_.insert(targetComponentId, std::move(transformed));
        return {true, 0, {}, {{QStringLiteral("interaction_state"), state}, {QStringLiteral("interaction_started"), true}}};
    }
    if (eventType == astra::ui::InputEventType::PointerMove || eventType == astra::ui::InputEventType::TouchUpdate) {
        if (session.machine().state() == astra::ui::InteractionState::Pressed) {
            operation = session.begin(astra::ui::InteractionState::Dragging);
            if (!operation.ok) return {false, operation.errorCode, operation.message, {}};
        }
        operation = session.dragBy(event.x - start.x(), event.y - start.y());
        if (operation.ok) operation = synchronizeWindowBounds(targetComponentId);
    } else if (eventType == astra::ui::InputEventType::PointerRelease || eventType == astra::ui::InputEventType::TouchEnd
               || eventType == astra::ui::InputEventType::GestureRelease) {
        if (session.machine().state() == astra::ui::InteractionState::Pressed) {
            operation = session.machine().transitionTo(astra::ui::InteractionState::Selected);
            if (operation.ok) operation = session.reset();
        } else {
            operation = session.commit();
        }
        if (operation.ok) {
            operation = synchronizeWindowBounds(targetComponentId);
        }
        if (operation.ok) {
            clearInteraction(targetComponentId);
            if (options_.statePersistence) {
                const auto saved = saveState();
                if (!saved.ok) return saved;
            }
        }
    } else {
        return {true, 0, {}, {{QStringLiteral("interaction_state"), interactionStateName(session.machine().state())}}};
    }
    if (!operation.ok) return {false, operation.errorCode, operation.message, {}};
    const QString state = interactionSessions_.contains(targetComponentId)
        ? interactionStateName(session.machine().state()) : QStringLiteral("IDLE");
    return {true, 0, {}, {{QStringLiteral("interaction_state"), state},
                          {QStringLiteral("interaction_completed"), state == QStringLiteral("IDLE")}}};
}

RuntimeResponse SpatialUIRuntime::cancelInteraction(const QJsonObject &params)
{
    if (!exactKeys(params, {QStringLiteral("component_id")}) || !isUuid(params.value(QStringLiteral("component_id")).toString())) {
        return {false, 5605, QStringLiteral("Invalid interaction cancel parameters"), {}};
    }
    const QString componentId = params.value(QStringLiteral("component_id")).toString();
    auto iterator = interactionSessions_.find(componentId);
    if (iterator == interactionSessions_.end()) return {false, 5605, QStringLiteral("No interaction is active"), {}};
    const auto cancelled = iterator.value()->cancel();
    if (!cancelled.ok) return {false, cancelled.errorCode, cancelled.message, {}};
    const auto synchronized = synchronizeWindowBounds(componentId);
    if (!synchronized.ok) return {false, synchronized.errorCode, synchronized.message, {}};
    clearInteraction(componentId);
    return {true, 0, {}, {{QStringLiteral("component_id"), componentId},
                          {QStringLiteral("interaction_state"), QStringLiteral("CANCELLED")},
                          {QStringLiteral("interaction_cancelled"), true}, {QStringLiteral("rollback"), true}}};
}

RuntimeResponse SpatialUIRuntime::applyWindowInteraction(const QString &method,
                                                         const QJsonObject &params,
                                                         astra::ui::SpatialWindow &window,
                                                         astra::ui::SpatialUIComponent &component)
{
    astra::ui::InteractionSession session {component, astra::ui::SpatialBounds {0.0, 0.0, 1280.0, 720.0}};
    auto result = session.press();
    if (!result.ok) return {false, result.errorCode, result.message, {}};
    const auto current = component.bounds();
    const bool resizing = method.endsWith(QStringLiteral("resize"));
    result = session.begin(resizing ? astra::ui::InteractionState::Resizing : astra::ui::InteractionState::Dragging);
    if (!result.ok) return {false, result.errorCode, result.message, {}};
    if (resizing) {
        result = session.resizeBy(params.value(QStringLiteral("width")).toDouble() - current.width,
                                  params.value(QStringLiteral("height")).toDouble() - current.height);
    } else {
        result = session.dragBy(params.value(QStringLiteral("x")).toDouble() - current.x,
                                params.value(QStringLiteral("y")).toDouble() - current.y);
    }
    if (!result.ok) {
        static_cast<void>(session.cancel());
        return {false, result.errorCode, result.message, {}};
    }
    result = session.commit();
    if (!result.ok) return {false, result.errorCode, result.message, {}};
    const auto updatedBounds = component.bounds();
    result = resizing ? windows_.resizeWindow(window.windowId(), updatedBounds.width, updatedBounds.height)
                      : windows_.moveWindow(window.windowId(), updatedBounds.x, updatedBounds.y);
    if (!result.ok) {
        static_cast<void>(session.cancel());
        return {false, result.errorCode, result.message, {}};
    }
    if (options_.statePersistence) {
        const auto saved = saveState();
        if (!saved.ok) return saved;
    }
    return {true, 0, {}, {{QStringLiteral("interaction_state"), QStringLiteral("IDLE")},
                          {QStringLiteral("interaction_completed"), true}, {QStringLiteral("rollback"), false}}};
}

RuntimeResponse SpatialUIRuntime::removeComponent(const QJsonObject &params)
{
    if (!exactKeys(params, {QStringLiteral("component_id")})
        || !isUuid(params.value(QStringLiteral("component_id")).toString())) {
        return {false, 5103, QStringLiteral("Invalid remove parameters"), {}};
    }
    const QString id = params.value(QStringLiteral("component_id")).toString();
    auto *component = components_.find(id);
    if (!component) return {false, 5105, QStringLiteral("Component not found"), {}};
    if (component->lifecycleState() == astra::ui::ComponentLifecycleState::Focused) static_cast<void>(component->transitionTo(astra::ui::ComponentLifecycleState::Visible));
    if (component->isVisible()) static_cast<void>(component->transitionTo(astra::ui::ComponentLifecycleState::Hidden));
    if (component->lifecycleState() == astra::ui::ComponentLifecycleState::Hidden) static_cast<void>(component->transitionTo(astra::ui::ComponentLifecycleState::Detached));
    if (component->lifecycleState() == astra::ui::ComponentLifecycleState::Detached) static_cast<void>(component->transitionTo(astra::ui::ComponentLifecycleState::Destroyed));
    static_cast<void>(focus_.unregisterTarget(id));
    if (tasks_.find(id)) static_cast<void>(tasks_.remove(id));
    clearInteraction(id);
    // Windows must not outlive their component, otherwise persisted state can no longer be restored.
    for (const auto *window : windows_.windows()) {
        if (window->componentId() == id) static_cast<void>(windows_.removeWindow(window->windowId()));
    }
    const auto result = components_.remove(id);
    return {result.ok, result.errorCode, result.message, {{QStringLiteral("component_id"), id}}};
}

QString SpatialUIRuntime::interactionSourceKey(const astra::ui::InputEvent &event)
{
    return QStringLiteral("%1:%2").arg(static_cast<int>(event.sourceType)).arg(event.sourceId);
}

void SpatialUIRuntime::clearInteraction(const QString &componentId)
{
    interactionSessions_.remove(componentId);
    interactionStartPositions_.remove(componentId);
    for (auto iterator = interactionTargetsBySource_.begin(); iterator != interactionTargetsBySource_.end();) {
        if (iterator.value() == componentId) iterator = interactionTargetsBySource_.erase(iterator);
        else ++iterator;
    }
}

astra::ui::OperationResult SpatialUIRuntime::cancelInteractionForUnavailableComponent(const QString &componentId)
{
    const auto iterator = interactionSessions_.find(componentId);
    if (iterator != interactionSessions_.end()) {
        const auto cancelled = iterator.value()->cancel();
        if (!cancelled.ok) return cancelled;
        const auto synchronized = synchronizeWindowBounds(componentId);
        if (!synchronized.ok) return synchronized;
    }
    clearInteraction(componentId);
    return {true, 0, {}};
}

astra::ui::OperationResult SpatialUIRuntime::synchronizeWindowBounds(const QString &componentId)
{
    auto *window = windows_.findByComponentId(componentId);
    auto *component = components_.find(componentId);
    if (!window || !component) return {true, 0, {}};
    const auto componentBounds = component->bounds();
    auto result = windows_.moveWindow(window->windowId(), componentBounds.x, componentBounds.y);
    if (result.ok) result = windows_.resizeWindow(window->windowId(), componentBounds.width, componentBounds.height);
    return result;
}

RuntimeResponse SpatialUIRuntime::updateComponent(const QJsonObject &params)
{
    static const QSet<QString> allowed {QStringLiteral("component_id"), QStringLiteral("bounds"), QStringLiteral("visible"),
                                        QStringLiteral("privacy_level"), QStringLiteral("display_target")};
    if (params.size() < 2 || !isUuid(params.value(QStringLiteral("component_id")).toString())) {
        return {false, 5103, QStringLiteral("Invalid component update parameters"), {}};
    }
    for (auto iterator = params.begin(); iterator != params.end(); ++iterator) {
        if (!allowed.contains(iterator.key())) return {false, 5103, QStringLiteral("Invalid component update parameters"), {}};
    }
    auto *component = components_.find(params.value(QStringLiteral("component_id")).toString());
    if (!component) return {false, 5105, QStringLiteral("Component not found"), {}};

    std::optional<astra::ui::SpatialBounds> nextBounds;
    std::optional<astra::common::PrivacyLevel> nextPrivacy;
    std::optional<astra::ui::DisplayTarget> nextTarget;
    if (params.contains(QStringLiteral("bounds"))) {
        nextBounds = bounds(params.value(QStringLiteral("bounds")).toObject());
        if (!nextBounds) return {false, 5205, QStringLiteral("Invalid component bounds"), {}};
    }
    if (params.contains(QStringLiteral("visible")) && !params.value(QStringLiteral("visible")).isBool()) {
        return {false, 5103, QStringLiteral("Invalid component visibility"), {}};
    }
    if (params.contains(QStringLiteral("privacy_level"))) {
        nextPrivacy = astra::common::privacyLevelFromString(params.value(QStringLiteral("privacy_level")).toString().toStdString());
        if (!nextPrivacy) return {false, 5702, QStringLiteral("Invalid component privacy level"), {}};
    }
    if (params.contains(QStringLiteral("display_target"))) {
        nextTarget = displayTarget(params.value(QStringLiteral("display_target")).toString());
        if (!nextTarget) return {false, 5703, QStringLiteral("Invalid component display target"), {}};
    }

    if (nextBounds) {
        const auto result = component->setBounds(*nextBounds);
        if (!result.ok) return {false, result.errorCode, result.message, {}};
    }
    if (nextPrivacy) {
        const auto classification = astra::policy::ProjectionPolicyService::classifyFixture(
            component->componentId(), *nextPrivacy, component->privacyLevel(), options_.policyBindingKey);
        component->setPrivacyLevel(classification.privacyLevel);
    }
    if (nextTarget) component->setDisplayTarget(*nextTarget);
    if (params.contains(QStringLiteral("visible"))) {
        const bool visible = params.value(QStringLiteral("visible")).toBool();
        if (visible && !component->isVisible()) {
            const auto result = component->transitionTo(astra::ui::ComponentLifecycleState::Visible);
            if (!result.ok) return {false, result.errorCode, result.message, {}};
            static_cast<void>(focus_.setAvailable(component->componentId(), true));
        } else if (!visible && component->isVisible()) {
            const auto cancelled = cancelInteractionForUnavailableComponent(component->componentId());
            if (!cancelled.ok) return {false, cancelled.errorCode, cancelled.message, {}};
            if (component->lifecycleState() == astra::ui::ComponentLifecycleState::Focused) {
                const auto unfocused = component->transitionTo(astra::ui::ComponentLifecycleState::Visible);
                if (!unfocused.ok) return {false, unfocused.errorCode, unfocused.message, {}};
            }
            const auto result = component->transitionTo(astra::ui::ComponentLifecycleState::Hidden);
            if (!result.ok) return {false, result.errorCode, result.message, {}};
            static_cast<void>(focus_.setAvailable(component->componentId(), false));
        }
    }
    const auto currentBounds = component->bounds();
    return {true, 0, {}, {{QStringLiteral("component_id"), component->componentId()},
                          {QStringLiteral("visible"), component->isVisible()},
                          {QStringLiteral("bounds"), QJsonObject {{QStringLiteral("x"), currentBounds.x}, {QStringLiteral("y"), currentBounds.y},
                                                                  {QStringLiteral("width"), currentBounds.width}, {QStringLiteral("height"), currentBounds.height}}}}};
}

RuntimeResponse SpatialUIRuntime::openWindow(const QJsonObject &params)
{
    const QSet<QString> keys {QStringLiteral("window_id"), QStringLiteral("component_id"), QStringLiteral("bounds"),
                              QStringLiteral("display_target"), QStringLiteral("projection_target")};
    const auto windowBounds = bounds(params.value(QStringLiteral("bounds")).toObject());
    const auto target = displayTarget(params.value(QStringLiteral("display_target")).toString());
    if (!exactKeys(params, keys) || !windowBounds || !target
        || QUuid {params.value(QStringLiteral("window_id")).toString()}.isNull()
        || QUuid {params.value(QStringLiteral("component_id")).toString()}.isNull()
        || (!params.value(QStringLiteral("projection_target")).isNull() && !params.value(QStringLiteral("projection_target")).isString())) {
        return {false, 5205, QStringLiteral("Invalid window parameters"), {}};
    }
    auto *component = components_.find(params.value(QStringLiteral("component_id")).toString());
    if (!component) return {false, 5105, QStringLiteral("Window component not found"), {}};
    const QString projectionTarget = params.value(QStringLiteral("projection_target")).toString();
    if ((*target == astra::ui::DisplayTarget::Projection || *target == astra::ui::DisplayTarget::Both) && projectionTarget.isEmpty()) {
        return {false, 5206, QStringLiteral("Projection target is required"), {}};
    }
    astra::ui::SpatialWindowSpec spec;
    spec.windowId = params.value(QStringLiteral("window_id")).toString();
    spec.componentId = params.value(QStringLiteral("component_id")).toString();
    spec.bounds = *windowBounds;
    spec.displayTarget = *target;
    spec.projectionTarget = projectionTarget;
    spec.privacyLevel = component->privacyLevel();
    spec.focusScope = QStringLiteral("workspace");
    const auto created = windows_.createWindow(spec);
    if (!created.ok) return {false, created.errorCode, created.message, {}};
    const auto shown = windows_.showWindow(spec.windowId);
    if (!shown.ok) return {false, shown.errorCode, shown.message, {}};
    const auto bounded = windows_.find(spec.windowId)->bounds();
    const auto componentUpdated = component->setBounds(bounded);
    if (!componentUpdated.ok) return {false, componentUpdated.errorCode, componentUpdated.message, {}};
    component->setDisplayTarget(*target);
    return {true, 0, {}, {{QStringLiteral("window_id"), spec.windowId}, {QStringLiteral("component_id"), spec.componentId}}};
}

RuntimeResponse SpatialUIRuntime::updateWindow(const QString &method, const QJsonObject &params)
{
    QSet<QString> keys {QStringLiteral("window_id")};
    if (method.endsWith(QStringLiteral("move"))) keys.unite({QStringLiteral("x"), QStringLiteral("y")});
    if (method.endsWith(QStringLiteral("resize"))) keys.unite({QStringLiteral("width"), QStringLiteral("height")});
    if (method.endsWith(QStringLiteral("target"))) keys.unite({QStringLiteral("display_target"), QStringLiteral("projection_target")});
    if (!exactKeys(params, keys) || !isUuid(params.value(QStringLiteral("window_id")).toString())) {
        return {false, 5205, QStringLiteral("Invalid window operation parameters"), {}};
    }
    if (method.endsWith(QStringLiteral("move"))
        && (!params.value(QStringLiteral("x")).isDouble() || !params.value(QStringLiteral("y")).isDouble())) {
        return {false, 5204, QStringLiteral("Invalid window position"), {}};
    }
    if (method.endsWith(QStringLiteral("resize"))
        && (!params.value(QStringLiteral("width")).isDouble() || !params.value(QStringLiteral("height")).isDouble())) {
        return {false, 5205, QStringLiteral("Invalid window size"), {}};
    }
    const QString id = params.value(QStringLiteral("window_id")).toString();
    auto *window = windows_.find(id);
    if (!window) return {false, 5201, QStringLiteral("Window not found"), {}};
    auto *component = components_.find(window->componentId());
    if (!component) return {false, 5105, QStringLiteral("Window component not found"), {}};
    astra::ui::OperationResult result;
    QJsonObject interactionValue;
    std::optional<astra::ui::DisplayTarget> nextTarget;
    if (method.endsWith(QStringLiteral("target"))) {
        nextTarget = displayTarget(params.value(QStringLiteral("display_target")).toString());
        if (!nextTarget || (!params.value(QStringLiteral("projection_target")).isNull()
                            && !params.value(QStringLiteral("projection_target")).isString())) {
            return {false, 5206, QStringLiteral("Invalid display target"), {}};
        }
    }
    if (method.endsWith(QStringLiteral("move")) || method.endsWith(QStringLiteral("resize"))) {
        const auto interaction = applyWindowInteraction(method, params, *window, *component);
        if (!interaction.ok) return interaction;
        interactionValue = interaction.value;
        result = {true, 0, {}};
    } else if (method.endsWith(QStringLiteral("hide"))) {
        result = windows_.hideWindow(id);
    } else if (method.endsWith(QStringLiteral("show"))) {
        result = windows_.showWindow(id);
    } else if (method.endsWith(QStringLiteral("target"))) {
        result = windows_.changeDisplayTarget(id, *nextTarget, params.value(QStringLiteral("projection_target")).toString());
    } else if (method.endsWith(QStringLiteral("restore"))) {
        result = windows_.restoreDefaultPosition(id);
    } else {
        result = windows_.closeWindow(id);
    }
    if (!result.ok) return {false, result.errorCode, result.message, {}};
    if (method.endsWith(QStringLiteral("close")) || method.endsWith(QStringLiteral("hide"))) {
        const auto cancelled = cancelInteractionForUnavailableComponent(component->componentId());
        if (!cancelled.ok) return {false, cancelled.errorCode, cancelled.message, {}};
        if (component->lifecycleState() == astra::ui::ComponentLifecycleState::Focused) {
            static_cast<void>(component->transitionTo(astra::ui::ComponentLifecycleState::Visible));
        }
        if (component->isVisible()) static_cast<void>(component->transitionTo(astra::ui::ComponentLifecycleState::Hidden));
        static_cast<void>(focus_.setAvailable(component->componentId(), false));
    } else if (method.endsWith(QStringLiteral("show"))) {
        if (!component->isVisible()) {
            const auto shown = component->transitionTo(astra::ui::ComponentLifecycleState::Visible);
            if (!shown.ok) return {false, shown.errorCode, shown.message, {}};
        }
        static_cast<void>(focus_.setAvailable(component->componentId(), true));
    } else if (method.endsWith(QStringLiteral("target"))) {
        component->setDisplayTarget(*nextTarget);
    } else {
        const auto componentUpdated = component->setBounds(window->bounds());
        if (!componentUpdated.ok) return {false, componentUpdated.errorCode, componentUpdated.message, {}};
    }
    const auto currentBounds = window->bounds();
    QJsonObject value {{QStringLiteral("window_id"), id}, {QStringLiteral("component_id"), component->componentId()},
                       {QStringLiteral("visible"), component->isVisible()}, {QStringLiteral("display_target"), displayTargetName(component->displayTarget())},
                       {QStringLiteral("bounds"), QJsonObject {{QStringLiteral("x"), currentBounds.x}, {QStringLiteral("y"), currentBounds.y},
                                                               {QStringLiteral("width"), currentBounds.width}, {QStringLiteral("height"), currentBounds.height}}}};
    for (auto iterator = interactionValue.begin(); iterator != interactionValue.end(); ++iterator) value.insert(iterator.key(), iterator.value());
    return {true, 0, {}, value};
}

RuntimeResponse SpatialUIRuntime::applyLayout(const QJsonObject &params)
{
    const QSet<QString> keys {QStringLiteral("mode"), QStringLiteral("direction"), QStringLiteral("component_ids"),
                              QStringLiteral("safe_area"), QStringLiteral("spacing"), QStringLiteral("margin"),
                              QStringLiteral("columns"), QStringLiteral("radial_radius"), QStringLiteral("anchor")};
    const auto mode = layoutMode(params.value(QStringLiteral("mode")).toString());
    const auto direction = layoutDirection(params.value(QStringLiteral("direction")).toString());
    const auto safeArea = bounds(params.value(QStringLiteral("safe_area")).toObject());
    if (!exactKeys(params, keys) || !mode || !direction || !safeArea
        || !params.value(QStringLiteral("component_ids")).isArray()
        || !params.value(QStringLiteral("spacing")).isDouble() || !params.value(QStringLiteral("margin")).isDouble()
        || !params.value(QStringLiteral("columns")).isDouble() || !params.value(QStringLiteral("radial_radius")).isDouble()
        || (!params.value(QStringLiteral("anchor")).isNull() && !params.value(QStringLiteral("anchor")).isObject())) {
        return {false, 5302, QStringLiteral("Invalid layout parameters"), {}};
    }
    astra::ui::LayoutRequest request;
    request.mode = *mode;
    request.direction = *direction;
    request.safeArea = *safeArea;
    request.spacing = params.value(QStringLiteral("spacing")).toDouble();
    request.margin = params.value(QStringLiteral("margin")).toDouble();
    request.columns = params.value(QStringLiteral("columns")).toInt();
    request.radialRadius = params.value(QStringLiteral("radial_radius")).toDouble();
    if (params.value(QStringLiteral("anchor")).isObject()) {
        const auto anchor = bounds(params.value(QStringLiteral("anchor")).toObject());
        if (!anchor) return {false, 5302, QStringLiteral("Invalid layout anchor"), {}};
        request.anchor = *anchor;
    }
    for (const auto &idValue : params.value(QStringLiteral("component_ids")).toArray()) {
        if (!idValue.isString() || !isUuid(idValue.toString())) return {false, 5302, QStringLiteral("Invalid layout component id"), {}};
        auto *component = components_.find(idValue.toString());
        if (!component) return {false, 5105, QStringLiteral("Layout component not found"), {}};
        request.items.append({component->componentId(), component->bounds()});
    }
    const auto result = layout_.apply(request);
    if (!result.ok) return {false, result.errorCode, result.message, {}};
    QJsonArray placements;
    for (const auto &placement : result.placements) {
        auto *component = components_.find(placement.componentId);
        if (!component) return {false, 5105, QStringLiteral("Layout component disappeared"), {}};
        const auto updated = component->setBounds(placement.bounds);
        if (!updated.ok) return {false, updated.errorCode, updated.message, {}};
        placements.append(QJsonObject {{QStringLiteral("component_id"), placement.componentId},
                                       {QStringLiteral("bounds"), QJsonObject {{QStringLiteral("x"), placement.bounds.x},
                                                                               {QStringLiteral("y"), placement.bounds.y},
                                                                               {QStringLiteral("width"), placement.bounds.width},
                                                                               {QStringLiteral("height"), placement.bounds.height}}}});
    }
    layoutMode_ = *mode;
    return {true, 0, {}, {{QStringLiteral("mode"), layoutModeName(layoutMode_)}, {QStringLiteral("placements"), placements}}};
}

RuntimeResponse SpatialUIRuntime::createNotification(const QJsonObject &params)
{
    const QSet<QString> keys {QStringLiteral("schema_version"), QStringLiteral("notification_id"), QStringLiteral("title"),
                              QStringLiteral("message"), QStringLiteral("severity"), QStringLiteral("privacy_level"),
                              QStringLiteral("display_target"), QStringLiteral("timeout_ms"), QStringLiteral("requires_action"),
                              QStringLiteral("actions"), QStringLiteral("created_at"), QStringLiteral("expires_at")};
    const auto severity = notificationSeverity(params.value(QStringLiteral("severity")).toString());
    const auto privacy = astra::common::privacyLevelFromString(params.value(QStringLiteral("privacy_level")).toString().toStdString());
    const auto target = displayTarget(params.value(QStringLiteral("display_target")).toString());
    const auto createdAt = QDateTime::fromString(params.value(QStringLiteral("created_at")).toString(), Qt::ISODateWithMs);
    QDateTime expiresAt;
    if (params.value(QStringLiteral("expires_at")).isString()) {
        expiresAt = QDateTime::fromString(params.value(QStringLiteral("expires_at")).toString(), Qt::ISODateWithMs);
    }
    if (!exactKeys(params, keys) || params.value(QStringLiteral("schema_version")).toString() != QStringLiteral("1.0")
        || QUuid {params.value(QStringLiteral("notification_id")).toString()}.isNull()
        || params.value(QStringLiteral("title")).toString().isEmpty() || params.value(QStringLiteral("message")).toString().isEmpty()
        || !severity || !privacy || !target || !params.value(QStringLiteral("timeout_ms")).isDouble()
        || params.value(QStringLiteral("timeout_ms")).toDouble() < 0.0
        || params.value(QStringLiteral("timeout_ms")).toDouble() > 86400000.0
        || !params.value(QStringLiteral("requires_action")).isBool() || !params.value(QStringLiteral("actions")).isArray()
        || !createdAt.isValid() || createdAt.offsetFromUtc() != 0
        || (!params.value(QStringLiteral("expires_at")).isNull() && (!expiresAt.isValid() || expiresAt < createdAt))) {
        return {false, 5103, QStringLiteral("Invalid notification parameters"), {}};
    }
    QStringList actions;
    for (const auto &action : params.value(QStringLiteral("actions")).toArray()) {
        if (!action.isString() || action.toString().isEmpty()) return {false, 5103, QStringLiteral("Invalid notification action"), {}};
        actions.append(action.toString());
    }
    astra::ui::NotificationSpec spec;
    spec.notificationId = params.value(QStringLiteral("notification_id")).toString();
    spec.title = params.value(QStringLiteral("title")).toString();
    spec.message = params.value(QStringLiteral("message")).toString();
    spec.severity = *severity;
    spec.privacyLevel = classifyFixture(spec.notificationId, *privacy).privacyLevel;
    spec.displayTarget = *target;
    spec.timeoutMs = params.value(QStringLiteral("timeout_ms")).toInt();
    spec.requiresAction = params.value(QStringLiteral("requires_action")).toBool();
    spec.actions = actions;
    spec.createdAt = createdAt;
    spec.expiresAt = expiresAt;
    const auto notificationCreated = notifications_.create(spec);
    if (!notificationCreated.ok) return {false, notificationCreated.errorCode, notificationCreated.message, {}};

    astra::ui::ComponentSpec componentSpec;
    componentSpec.id = spec.notificationId;
    componentSpec.type = astra::ui::ComponentType::Notification;
    componentSpec.bounds = {24.0, 24.0 + static_cast<double>(notifications_.size() - 1) * 136.0, 360.0, 120.0};
    componentSpec.zOrder = 10000 + static_cast<int>(notifications_.size());
    componentSpec.focusable = spec.severity == astra::ui::NotificationSeverity::Critical;
    componentSpec.interactive = spec.requiresAction || spec.severity == astra::ui::NotificationSeverity::Critical;
    componentSpec.privacyLevel = spec.privacyLevel;
    componentSpec.displayTarget = spec.displayTarget;
    componentSpec.accessibilityLabel = spec.title;
    auto componentCreated = components_.create(componentSpec, astra::ui::Principal::Application);
    if (!componentCreated.ok) {
        static_cast<void>(notifications_.remove(spec.notificationId));
        return {false, componentCreated.errorCode, componentCreated.message, {}};
    }
    static_cast<void>(componentCreated.component->transitionTo(astra::ui::ComponentLifecycleState::Attached));
    static_cast<void>(componentCreated.component->transitionTo(astra::ui::ComponentLifecycleState::Visible));
    static_cast<void>(focus_.registerTarget({spec.notificationId, QStringLiteral("workspace"), true, componentSpec.interactive,
                                            componentSpec.focusable, componentSpec.zOrder}));
    QString previousFocus;
    if (spec.severity == astra::ui::NotificationSeverity::Critical) {
        const auto previous = focus_.record(QStringLiteral("workspace"));
        previousFocus = previous.componentId;
        criticalFocusRestoreChain_.append({spec.notificationId, previous});
        const auto focused = focus_.requestFocus(spec.notificationId, astra::ui::FocusType::System,
                                                 astra::ui::FocusPriority::System, QStringLiteral("critical notification"));
        if (!focused.ok) return {false, focused.errorCode, focused.message, {}};
    }
    return {true, 0, {}, {{QStringLiteral("notification_id"), spec.notificationId},
                          {QStringLiteral("component_id"), spec.notificationId},
                          {QStringLiteral("notification_count"), notifications_.size()},
                          {QStringLiteral("focus_component_id"), focus_.owner(QStringLiteral("workspace"))},
                          {QStringLiteral("previous_focus_component_id"), previousFocus},
                          {QStringLiteral("focus_preempted"), spec.severity == astra::ui::NotificationSeverity::Critical},
                          {QStringLiteral("reason"), spec.severity == astra::ui::NotificationSeverity::Critical
                              ? QStringLiteral("critical notification") : QString {}}}};
}

RuntimeResponse SpatialUIRuntime::expireNotifications(const QDateTime &now, const QString &traceId, const QString &requestId)
{
    struct ExpiredNotification {
        QString id;
        astra::common::PrivacyLevel privacyLevel;
    };
    QVector<ExpiredNotification> expired;
    for (const auto *component : components_.components()) {
        const auto *notification = notifications_.find(component->componentId());
        if (notification && notification->expiredAt(now)) {
            expired.append({component->componentId(), notification->spec().privacyLevel});
        }
    }
    if (expired.isEmpty()) return {true, 0, {}, {{QStringLiteral("expired_notifications"), QJsonArray {}}}};

    QJsonArray expiredValues;
    for (const auto &entry : expired) {
        const auto cleared = clearNotification({{QStringLiteral("notification_id"), entry.id}});
        if (!cleared.ok) return cleared;
        expiredValues.append(QJsonObject {
            {QStringLiteral("notification_id"), entry.id},
            {QStringLiteral("component_id"), entry.id},
            {QStringLiteral("privacy_level"), QString::fromLatin1(astra::common::privacyLevelToString(entry.privacyLevel).data())},
            {QStringLiteral("focus_restored"), cleared.value.value(QStringLiteral("focus_restored"))},
            {QStringLiteral("restored_focus_component_id"), cleared.value.value(QStringLiteral("restored_focus_component_id"))},
            {QStringLiteral("reason"), cleared.value.value(QStringLiteral("reason"))},
        });
    }
    auto synchronized = synchronizeProjection({}, traceId, requestId);
    if (!synchronized.ok) return synchronized;
    synchronized.value.insert(QStringLiteral("expired_notifications"), expiredValues);
    return synchronized;
}

RuntimeResponse SpatialUIRuntime::performMaintenance(const QDateTime &now,
                                                     const QString &traceId,
                                                     const QString &requestId,
                                                     bool autosaveDue)
{
    if (!now.isValid()) return {false, 5103, QStringLiteral("Invalid maintenance timestamp"), {}};
    auto result = expireNotifications(now, traceId, requestId);
    if (!result.ok) return result;
    result.value.insert(QStringLiteral("autosaved"), false);
    if (autosaveDue && options_.statePersistence) {
        const auto saved = saveState();
        if (!saved.ok) return saved;
        result.value.insert(QStringLiteral("autosaved"), true);
        result.value.insert(QStringLiteral("state_path"), saved.value.value(QStringLiteral("state_path")));
    }
    return result;
}

RuntimeResponse SpatialUIRuntime::saveState()
{
    if (!options_.statePersistence) return {false, 5801, QStringLiteral("State persistence is disabled"), {}};
    astra::ui::UIStateSnapshot snapshot;
    snapshot.layoutMode = layoutMode_;
    // Task surfaces and notifications are regenerated by their sources; their content is
    // not persisted, so restoring them would only produce empty shells.
    QSet<QString> skippedComponentIds;
    for (const auto *component : components_.components()) {
        if (tasks_.find(component->componentId()) || notifications_.find(component->componentId())) {
            skippedComponentIds.insert(component->componentId());
            continue;
        }
        if (astra::ui::isSystemOnly(component->componentType())) continue;
        snapshot.components.append({component->componentId(), component->componentType(), component->bounds(), component->zOrder(),
                                    component->isVisible(), component->isFocusable(), component->isInteractive(), component->displayTarget()});
        if (component->isVisible()) snapshot.visibleComponentIds.append(component->componentId());
    }
    for (const auto *window : windows_.windows()) {
        if (window->state() == astra::ui::SpatialWindowState::Closed || skippedComponentIds.contains(window->componentId())) continue;
        const bool visible = window->state() == astra::ui::SpatialWindowState::Visible;
        snapshot.windows.append({window->windowId(), window->componentId(), window->bounds(), visible, window->displayTarget(),
                                 window->projectionTarget(), window->anchorId(), window->focusScope()});
    }
    snapshot.selectedTab = selectedTab_;
    const QString focusOwner = focus_.owner(QStringLiteral("workspace"));
    snapshot.focusRestoreComponentId = skippedComponentIds.contains(focusOwner) ? QString {} : focusOwner;
    snapshot.panelOrder = panelOrder_;
    const auto result = stateStore_.save(snapshot);
    return {result.ok, result.errorCode, result.message, {{QStringLiteral("state_path"), stateStore_.path()}}};
}

RuntimeResponse SpatialUIRuntime::clearNotification(const QJsonObject &params)
{
    if (!exactKeys(params, {QStringLiteral("notification_id")})
        || !isUuid(params.value(QStringLiteral("notification_id")).toString())) {
        return {false, 5103, QStringLiteral("Invalid notification clear parameters"), {}};
    }
    const QString id = params.value(QStringLiteral("notification_id")).toString();
    const auto *notification = notifications_.find(id);
    if (!notification) return {false, 5105, QStringLiteral("Notification not found"), {}};
    const bool restoreFocus = notification->requestsSystemFocus();
    const QString previousFocus = focus_.owner(QStringLiteral("workspace"));
    const bool wasFocused = previousFocus == id;
    QString restoredFocus;
    bool focusRestored = false;
    const auto removedComponent = removeComponent({{QStringLiteral("component_id"), id}});
    if (!removedComponent.ok) return removedComponent;
    const auto removed = notifications_.remove(id);
    if (removed.ok && restoreFocus) {
        qsizetype chainIndex = -1;
        for (qsizetype index = 0; index < criticalFocusRestoreChain_.size(); ++index) {
            if (criticalFocusRestoreChain_.at(index).notificationId == id) {
                chainIndex = index;
                break;
            }
        }
        if (chainIndex >= 0) {
            const auto previous = criticalFocusRestoreChain_.at(chainIndex).previous;
            criticalFocusRestoreChain_.removeAt(chainIndex);
            for (qsizetype index = chainIndex; index < criticalFocusRestoreChain_.size(); ++index) {
                if (criticalFocusRestoreChain_[index].previous.componentId == id) criticalFocusRestoreChain_[index].previous = previous;
            }
            if (wasFocused) {
                if (previous.componentId.isEmpty()) {
                    const auto released = focus_.releaseFocus(QStringLiteral("workspace"), QStringLiteral("critical notification handled"));
                    if (!released.ok) return {false, 5504, QStringLiteral("Notification focus restore failed"), {}};
                    focusRestored = true;
                } else {
                    const auto focused = focus_.requestFocus(previous.componentId, previous.type, previous.priority,
                                                             QStringLiteral("critical notification handled"));
                    if (!focused.ok) return {false, 5504, QStringLiteral("Notification focus restore failed"), {}};
                    restoredFocus = focus_.owner(QStringLiteral("workspace"));
                    focusRestored = true;
                }
            }
        }
    }
    return {removed.ok, removed.errorCode, removed.message,
            {{QStringLiteral("notification_id"), id}, {QStringLiteral("component_id"), id},
             {QStringLiteral("notification_count"), notifications_.size()},
             {QStringLiteral("focus_component_id"), focus_.owner(QStringLiteral("workspace"))},
             {QStringLiteral("previous_focus_component_id"), previousFocus},
             {QStringLiteral("restored_focus_component_id"), restoredFocus},
             {QStringLiteral("focus_restored"), focusRestored},
             {QStringLiteral("reason"), restoreFocus ? QStringLiteral("critical notification handled") : QString {}}}};
}

} // namespace astra::spatial_ui::service
