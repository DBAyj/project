#include "astra/spatial_ui/service/SpatialUIRuntime.h"

#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QTemporaryDir>
#include <QtGlobal>

#include <algorithm>

using astra::spatial_ui::service::SpatialUIRuntime;

namespace {

void require(bool condition, const char *message)
{
    if (!condition) qFatal("P5 runtime command requirement failed: %s", message);
}

QJsonObject component(const QString &id, double x, double y, double width, double height)
{
    return {{QStringLiteral("component_id"), id},
            {QStringLiteral("component_type"), QStringLiteral("BUTTON")},
            {QStringLiteral("privacy_level"), QStringLiteral("PRIVATE_SCREEN_ONLY")},
            {QStringLiteral("accessibility_label"), QStringLiteral("Input target")},
            {QStringLiteral("bounds"), QJsonObject {{QStringLiteral("x"), x}, {QStringLiteral("y"), y},
                                                     {QStringLiteral("width"), width}, {QStringLiteral("height"), height}}},
            {QStringLiteral("display_target"), QStringLiteral("PHONE")}};
}

QJsonObject task(const QString &id)
{
    return {{QStringLiteral("schema_version"), QStringLiteral("1.0")},
            {QStringLiteral("task_id"), id},
            {QStringLiteral("title"), QStringLiteral("Fixture task")},
            {QStringLiteral("summary"), QStringLiteral("Task lifecycle fixture")},
            {QStringLiteral("intent_type"), QStringLiteral("open_task_surface")},
            {QStringLiteral("confidence"), 1.0},
            {QStringLiteral("execution_strategy"), QStringLiteral("fixture_adapter")},
            {QStringLiteral("privacy_level"), QStringLiteral("PRIVATE_SCREEN_ONLY")},
            {QStringLiteral("bounds"), QJsonObject {{QStringLiteral("x"), 20.0}, {QStringLiteral("y"), 20.0},
                                                     {QStringLiteral("width"), 240.0}, {QStringLiteral("height"), 120.0}}},
            {QStringLiteral("display_target"), QStringLiteral("PHONE")},
            {QStringLiteral("accessibility_label"), QStringLiteral("Fixture task")}};
}

} // namespace

int main()
{
    QTemporaryDir temporary;
    require(temporary.isValid(), "temporary directory");
    const QString statePath = temporary.filePath(QStringLiteral("ui-state.json"));
    SpatialUIRuntime runtime {statePath};
    const QString componentId = QStringLiteral("5f89cc30-7060-4c9c-8662-0e8bc686c547");
    const QString windowId = QStringLiteral("d13d0ba9-8de1-4c38-97ae-09f9d56e9935");
    require(!runtime.dispatch(QStringLiteral("spatial_ui.component.create"), component(QStringLiteral("not-a-uuid"), 0.0, 0.0, 100.0, 100.0)).ok,
            "non-UUID component rejected");
    require(runtime.dispatch(QStringLiteral("spatial_ui.component.create"), component(componentId, 20.0, 30.0, 200.0, 100.0)).ok,
            "component created");

    const QJsonObject event {
        {QStringLiteral("schema_version"), QStringLiteral("1.0")},
        {QStringLiteral("event_id"), QStringLiteral("fa5078de-7ab0-4f7e-9f37-5bd18354b6d2")},
        {QStringLiteral("event_type"), QStringLiteral("POINTER_PRESS")},
        {QStringLiteral("source_type"), QStringLiteral("MOUSE")},
        {QStringLiteral("source_id"), QStringLiteral("primary-mouse")},
        {QStringLiteral("target_component_id"), QJsonValue {QJsonValue::Null}},
        {QStringLiteral("position"), QJsonObject {{QStringLiteral("coordinate_system"), QStringLiteral("PHONE_VIEW")},
                                                  {QStringLiteral("x"), 50.0}, {QStringLiteral("y"), 60.0}}},
        {QStringLiteral("modifiers"), QJsonArray {}},
        {QStringLiteral("timestamp"), QStringLiteral("2026-07-15T12:00:00.000Z")},
    };
    const auto routed = runtime.dispatch(QStringLiteral("spatial_ui.input"), event);
    require(routed.ok, "input routed");
    require(routed.value.value(QStringLiteral("target_component_id")).toString() == componentId, "input target returned");
    require(!routed.value.value(QStringLiteral("blocked")).toBool(), "ordinary input not blocked");

    const auto hidden = runtime.dispatch(QStringLiteral("spatial_ui.component.update"),
                                         {{QStringLiteral("component_id"), componentId}, {QStringLiteral("visible"), false}});
    require(hidden.ok, "component hidden through update");
    require(!hidden.value.value(QStringLiteral("visible")).toBool(), "hidden state returned");
    const auto hiddenRoute = runtime.dispatch(QStringLiteral("spatial_ui.input"), event);
    require(!hiddenRoute.ok && hiddenRoute.errorCode == 5404, "hidden component skipped by input router");

    const auto shown = runtime.dispatch(QStringLiteral("spatial_ui.component.update"),
                                        {{QStringLiteral("component_id"), componentId},
                                         {QStringLiteral("visible"), true},
                                         {QStringLiteral("bounds"), QJsonObject {{QStringLiteral("x"), 40.0}, {QStringLiteral("y"), 40.0},
                                                                                  {QStringLiteral("width"), 240.0}, {QStringLiteral("height"), 120.0}}}});
    require(shown.ok, "component shown through update");
    require(shown.value.value(QStringLiteral("visible")).toBool(), "visible state returned");
    require(runtime.dispatch(QStringLiteral("spatial_ui.input"), event).ok, "shown component restored to input router");
    QJsonObject targetedPointer = event;
    targetedPointer.insert(QStringLiteral("event_id"), QStringLiteral("ce3c2692-dd77-46c2-a608-7624c2a09f04"));
    targetedPointer.insert(QStringLiteral("target_component_id"), componentId);
    targetedPointer.insert(QStringLiteral("position"), QJsonObject {{QStringLiteral("coordinate_system"), QStringLiteral("PHONE_VIEW")},
                                                                     {QStringLiteral("x"), 900.0}, {QStringLiteral("y"), 600.0}});
    const auto directlyRouted = runtime.dispatch(QStringLiteral("spatial_ui.input"), targetedPointer);
    require(directlyRouted.ok && directlyRouted.value.value(QStringLiteral("target_component_id")).toString() == componentId,
            "Shell pointer input preserves its explicit component target");
    const auto privacyDowngrade = runtime.dispatch(
        QStringLiteral("spatial_ui.component.update"),
        {{QStringLiteral("component_id"), componentId}, {QStringLiteral("privacy_level"), QStringLiteral("PUBLIC")}});
    require(privacyDowngrade.ok, "privacy update evaluated by policy");
    const auto privacyState = runtime.dispatch(QStringLiteral("spatial_ui.components"), {}).value
                                  .value(QStringLiteral("components")).toArray().first().toObject();
    require(privacyState.value(QStringLiteral("privacy_level")).toString() == QStringLiteral("PRIVATE_SCREEN_ONLY"),
            "caller cannot downgrade component privacy");

    const QString secondId = QStringLiteral("ac51ebf4-5462-4f82-8082-fc194acb1160");
    require(runtime.dispatch(QStringLiteral("spatial_ui.component.create"), component(secondId, 300.0, 30.0, 160.0, 100.0)).ok,
            "second component created");
    require(runtime.dispatch(QStringLiteral("spatial_ui.focus"),
                             {{QStringLiteral("component_id"), componentId},
                              {QStringLiteral("reason"), QStringLiteral("keyboard traversal baseline")}}).ok,
            "keyboard traversal baseline focused");
    QJsonObject keyboardFocus = event;
    keyboardFocus.insert(QStringLiteral("event_id"), QStringLiteral("c4c3e21c-e61d-483a-842b-9e946c3acd13"));
    keyboardFocus.insert(QStringLiteral("event_type"), QStringLiteral("SYSTEM_FOCUS"));
    keyboardFocus.insert(QStringLiteral("source_type"), QStringLiteral("KEYBOARD"));
    keyboardFocus.insert(QStringLiteral("source_id"), QStringLiteral("primary-keyboard"));
    keyboardFocus.insert(QStringLiteral("target_component_id"), secondId);
    keyboardFocus.insert(QStringLiteral("position"), QJsonValue {QJsonValue::Null});
    require(runtime.dispatch(QStringLiteral("spatial_ui.input"), keyboardFocus).ok
                && runtime.status().value(QStringLiteral("focus_component_id")).toString() == secondId,
            "unified keyboard focus moves between ordinary components");
    keyboardFocus.insert(QStringLiteral("event_id"), QStringLiteral("c8fce0de-c4fa-45a4-b7a8-1ff3fe5299ca"));
    keyboardFocus.insert(QStringLiteral("target_component_id"), componentId);
    require(runtime.dispatch(QStringLiteral("spatial_ui.input"), keyboardFocus).ok
                && runtime.status().value(QStringLiteral("focus_component_id")).toString() == componentId,
            "unified keyboard focus returns through focus manager");
    const QString childId = QStringLiteral("dbb38af9-5be7-4f43-902d-f43e8cc42aaa");
    auto childParams = component(childId, 8.0, 8.0, 80.0, 40.0);
    childParams.insert(QStringLiteral("parent_id"), componentId);
    childParams.insert(QStringLiteral("children"), QJsonArray {});
    require(runtime.dispatch(QStringLiteral("spatial_ui.component.create"), childParams).ok,
            "child component created");
    auto hierarchy = runtime.dispatch(QStringLiteral("spatial_ui.components"), {})
                         .value.value(QStringLiteral("components")).toArray();
    QJsonObject parentValue;
    QJsonObject childValue;
    for (const auto &value : hierarchy) {
        const auto object = value.toObject();
        if (object.value(QStringLiteral("component_id")).toString() == componentId) parentValue = object;
        if (object.value(QStringLiteral("component_id")).toString() == childId) childValue = object;
    }
    require(parentValue.value(QStringLiteral("children")).toArray() == QJsonArray {childId}
                && childValue.value(QStringLiteral("parent_id")).toString() == componentId,
            "runtime component hierarchy is linked in both directions");
    require(runtime.dispatch(QStringLiteral("spatial_ui.component.remove"),
                             {{QStringLiteral("component_id"), childId}}).ok,
            "child component removed");
    hierarchy = runtime.dispatch(QStringLiteral("spatial_ui.components"), {})
                    .value.value(QStringLiteral("components")).toArray();
    for (const auto &value : hierarchy) {
        const auto object = value.toObject();
        if (object.value(QStringLiteral("component_id")).toString() == componentId) {
            require(object.value(QStringLiteral("children")).toArray().isEmpty(),
                    "removing a child updates its parent");
        }
    }
    const auto layout = runtime.dispatch(
        QStringLiteral("spatial_ui.layout.apply"),
        {{QStringLiteral("mode"), QStringLiteral("GRID")},
         {QStringLiteral("direction"), QStringLiteral("HORIZONTAL")},
         {QStringLiteral("component_ids"), QJsonArray {componentId, secondId}},
         {QStringLiteral("safe_area"), QJsonObject {{QStringLiteral("x"), 0.0}, {QStringLiteral("y"), 0.0},
                                                    {QStringLiteral("width"), 600.0}, {QStringLiteral("height"), 400.0}}},
         {QStringLiteral("spacing"), 12.0}, {QStringLiteral("margin"), 20.0},
         {QStringLiteral("columns"), 2}, {QStringLiteral("radial_radius"), 100.0},
         {QStringLiteral("anchor"), QJsonValue {QJsonValue::Null}}});
    require(layout.ok, "layout applied");
    require(layout.value.value(QStringLiteral("mode")).toString() == QStringLiteral("GRID"), "layout mode returned");
    require(layout.value.value(QStringLiteral("placements")).toArray().size() == 2, "layout placements returned");

    require(runtime.dispatch(QStringLiteral("spatial_ui.component.update"),
                             {{QStringLiteral("component_id"), componentId},
                              {QStringLiteral("bounds"), QJsonObject {{QStringLiteral("x"), 10.0}, {QStringLiteral("y"), 10.0},
                                                                       {QStringLiteral("width"), 100.0}, {QStringLiteral("height"), 100.0}}}}).ok,
            "component positioned for window test");
    require(runtime.dispatch(QStringLiteral("spatial_ui.window.open"),
                             {{QStringLiteral("window_id"), windowId},
                              {QStringLiteral("component_id"), componentId},
                              {QStringLiteral("bounds"), QJsonObject {{QStringLiteral("x"), 10.0}, {QStringLiteral("y"), 10.0},
                                                                       {QStringLiteral("width"), 100.0}, {QStringLiteral("height"), 100.0}}},
                              {QStringLiteral("display_target"), QStringLiteral("PHONE")},
                              {QStringLiteral("projection_target"), QJsonValue {QJsonValue::Null}}}).ok,
            "window opened");
    const auto moved = runtime.dispatch(QStringLiteral("spatial_ui.window.move"),
                                        {{QStringLiteral("window_id"), windowId},
                                         {QStringLiteral("x"), 400.0}, {QStringLiteral("y"), 200.0}});
    require(moved.ok, "window moved");
    require(moved.value.value(QStringLiteral("interaction_completed")).toBool(), "window move uses interaction state machine");
    QJsonObject movedEvent = event;
    movedEvent.insert(QStringLiteral("event_id"), QStringLiteral("34915fd2-eb4a-4ba3-8287-ac2d275b39bd"));
    movedEvent.insert(QStringLiteral("position"), QJsonObject {{QStringLiteral("coordinate_system"), QStringLiteral("PHONE_VIEW")},
                                                               {QStringLiteral("x"), 450.0}, {QStringLiteral("y"), 250.0}});
    const auto movedRoute = runtime.dispatch(QStringLiteral("spatial_ui.input"), movedEvent);
    require(movedRoute.ok && movedRoute.value.value(QStringLiteral("target_component_id")).toString() == componentId,
            "window move updates component input bounds");
    QJsonObject dragStart = movedEvent;
    dragStart.insert(QStringLiteral("event_id"), QStringLiteral("17d0a860-e8ef-4816-a52a-ec75ca2ff7da"));
    const auto interactionStarted = runtime.dispatch(QStringLiteral("spatial_ui.input"), dragStart);
    require(interactionStarted.ok && interactionStarted.value.value(QStringLiteral("interaction_started")).toBool(),
            "pointer press starts interaction state machine");
    QJsonObject dragMove = dragStart;
    dragMove.insert(QStringLiteral("event_id"), QStringLiteral("e504fbb0-8487-426a-8784-b9709a9ae6d9"));
    dragMove.insert(QStringLiteral("event_type"), QStringLiteral("POINTER_MOVE"));
    dragMove.insert(QStringLiteral("position"), QJsonObject {{QStringLiteral("coordinate_system"), QStringLiteral("PHONE_VIEW")},
                                                              {QStringLiteral("x"), 500.0}, {QStringLiteral("y"), 280.0}});
    require(runtime.dispatch(QStringLiteral("spatial_ui.input"), dragMove).ok, "pointer drag applies transform");
    const auto cancelledInteraction = runtime.dispatch(QStringLiteral("spatial_ui.interaction.cancel"),
                                                        {{QStringLiteral("component_id"), componentId}});
    require(cancelledInteraction.ok && cancelledInteraction.value.value(QStringLiteral("rollback")).toBool(),
            "interaction cancel rolls back transform");
    dragStart.insert(QStringLiteral("event_id"), QStringLiteral("7c1c1774-90a9-4c91-9cd7-bc26313d12f3"));
    require(runtime.dispatch(QStringLiteral("spatial_ui.input"), dragStart).ok, "second pointer drag starts");
    dragMove.insert(QStringLiteral("event_id"), QStringLiteral("a128233a-d393-4da3-9dc0-f42284a55ad7"));
    require(runtime.dispatch(QStringLiteral("spatial_ui.input"), dragMove).ok, "second pointer drag moves window");
    QJsonObject dragRelease = dragMove;
    dragRelease.insert(QStringLiteral("event_id"), QStringLiteral("35913f12-6913-4e62-84d1-baa1404f268b"));
    dragRelease.insert(QStringLiteral("event_type"), QStringLiteral("POINTER_RELEASE"));
    dragRelease.insert(QStringLiteral("position"), QJsonObject {{QStringLiteral("coordinate_system"), QStringLiteral("PHONE_VIEW")},
                                                                 {QStringLiteral("x"), 1200.0}, {QStringLiteral("y"), 680.0}});
    const auto releasedOutside = runtime.dispatch(QStringLiteral("spatial_ui.input"), dragRelease);
    require(releasedOutside.ok && releasedOutside.value.value(QStringLiteral("interaction_completed")).toBool(),
            "pointer release outside captured component commits interaction");
    const auto windowsAfterDrag = runtime.dispatch(QStringLiteral("spatial_ui.windows"), {}).value.value(QStringLiteral("windows")).toArray();
    require(windowsAfterDrag.size() == 1
                && windowsAfterDrag.at(0).toObject().value(QStringLiteral("bounds")).toObject().value(QStringLiteral("x")).toDouble() == 450.0,
            "pointer drag synchronizes window metadata");
    QFile autosavedState {statePath};
    require(autosavedState.open(QIODevice::ReadOnly), "pointer release autosaves runtime state");
    const QJsonObject autosavedDocument = QJsonDocument::fromJson(autosavedState.readAll()).object();
    const QJsonArray autosavedComponents = autosavedDocument.value(QStringLiteral("components")).toArray();
    const QJsonArray autosavedWindows = autosavedDocument.value(QStringLiteral("windows")).toArray();
    const auto savedComponent = std::find_if(autosavedComponents.cbegin(), autosavedComponents.cend(), [&componentId](const auto &value) {
        return value.toObject().value(QStringLiteral("component_id")).toString() == componentId;
    });
    const auto savedWindow = std::find_if(autosavedWindows.cbegin(), autosavedWindows.cend(), [&windowId](const auto &value) {
        return value.toObject().value(QStringLiteral("window_id")).toString() == windowId;
    });
    require(savedComponent != autosavedComponents.cend()
                && savedComponent->toObject().value(QStringLiteral("bounds")).toObject().value(QStringLiteral("x")).toDouble() == 450.0,
            "autosave contains committed component bounds");
    require(savedWindow != autosavedWindows.cend()
                && savedWindow->toObject().value(QStringLiteral("bounds")).toObject().value(QStringLiteral("x")).toDouble() == 450.0,
            "autosave contains committed window bounds");

    QJsonObject gestureGrab = movedEvent;
    gestureGrab.insert(QStringLiteral("event_id"), QStringLiteral("b5eb4bc3-ecce-4f10-9242-1d5ddf38c44a"));
    gestureGrab.insert(QStringLiteral("event_type"), QStringLiteral("GESTURE_GRAB"));
    gestureGrab.insert(QStringLiteral("source_type"), QStringLiteral("SIMULATED_GESTURE"));
    gestureGrab.insert(QStringLiteral("source_id"), QStringLiteral("fixture-gesture"));
    require(runtime.dispatch(QStringLiteral("spatial_ui.input"), gestureGrab).ok, "simulated gesture grabs component");
    QJsonObject gestureScale = gestureGrab;
    gestureScale.insert(QStringLiteral("event_id"), QStringLiteral("b100211c-8ab1-460a-9bcf-43fb4dc30243"));
    gestureScale.insert(QStringLiteral("event_type"), QStringLiteral("GESTURE_SCALE"));
    gestureScale.insert(QStringLiteral("interaction_value"), 1.25);
    const auto scaled = runtime.dispatch(QStringLiteral("spatial_ui.input"), gestureScale);
    require(scaled.ok && scaled.value.value(QStringLiteral("interaction_state")).toString() == QStringLiteral("SCALING"),
            "gesture scale uses explicit interaction value");
    QJsonObject gestureRelease = gestureGrab;
    gestureRelease.insert(QStringLiteral("event_id"), QStringLiteral("2e555b12-1a5d-4143-8d30-fd78d9816555"));
    gestureRelease.insert(QStringLiteral("event_type"), QStringLiteral("GESTURE_RELEASE"));
    require(runtime.dispatch(QStringLiteral("spatial_ui.input"), gestureRelease).value.value(QStringLiteral("interaction_completed")).toBool(),
            "simulated gesture commits through captured source");
    QJsonObject invalidGesture = gestureScale;
    invalidGesture.remove(QStringLiteral("interaction_value"));
    require(!runtime.dispatch(QStringLiteral("spatial_ui.input"), invalidGesture).ok,
            "transform gesture without interaction value rejected");
    require(runtime.dispatch(QStringLiteral("spatial_ui.window.hide"),
                             {{QStringLiteral("window_id"), windowId}}).ok,
            "window hidden");
    require(runtime.dispatch(QStringLiteral("spatial_ui.input"), movedEvent).value.value(QStringLiteral("target_component_id")).toString() != componentId,
            "hidden window removed from input routing");
    require(runtime.dispatch(QStringLiteral("spatial_ui.window.show"),
                             {{QStringLiteral("window_id"), windowId}}).ok,
            "window shown");
    require(runtime.dispatch(QStringLiteral("spatial_ui.input"), movedEvent).value.value(QStringLiteral("target_component_id")).toString() == componentId,
            "shown window restored to input routing");

    require(runtime.dispatch(QStringLiteral("spatial_ui.focus"),
                             {{QStringLiteral("component_id"), componentId}, {QStringLiteral("reason"), QStringLiteral("notification baseline")}}).ok,
            "baseline focus acquired");
    const QString expiredWarningId = QStringLiteral("1687c5e1-f408-47a0-a45a-bf36233ad818");
    require(runtime.dispatch(
                QStringLiteral("spatial_ui.notification.create"),
                {{QStringLiteral("schema_version"), QStringLiteral("1.0")},
                 {QStringLiteral("notification_id"), expiredWarningId}, {QStringLiteral("title"), QStringLiteral("Expired warning")},
                 {QStringLiteral("message"), QStringLiteral("Runtime expiry fixture")},
                 {QStringLiteral("severity"), QStringLiteral("WARNING")}, {QStringLiteral("privacy_level"), QStringLiteral("PUBLIC")},
                 {QStringLiteral("display_target"), QStringLiteral("PHONE")}, {QStringLiteral("timeout_ms"), 1},
                 {QStringLiteral("requires_action"), false}, {QStringLiteral("actions"), QJsonArray {}},
                 {QStringLiteral("created_at"), QStringLiteral("2000-01-01T00:00:00.000Z")},
                 {QStringLiteral("expires_at"), QStringLiteral("2000-01-01T00:00:00.001Z")}}).ok,
            "expired warning accepted before runtime cleanup boundary");
    require(runtime.performMaintenance(QDateTime::currentDateTimeUtc(),
                                       QStringLiteral("98b998ba-e9e5-49f7-af4d-ce85d91a685c"),
                                       QStringLiteral("acfe1ec3-18f5-4489-88bc-582c2a2b6274"), false).ok,
            "runtime maintenance expires warning without a follow-up command");
    const auto componentsAfterExpiry = runtime.dispatch(QStringLiteral("spatial_ui.components"), {})
                                           .value.value(QStringLiteral("components")).toArray();
    bool expiredComponentRetained = false;
    for (const auto &componentValue : componentsAfterExpiry) {
        expiredComponentRetained = expiredComponentRetained
            || componentValue.toObject().value(QStringLiteral("component_id")).toString() == expiredWarningId;
    }
    require(!expiredComponentRetained, "runtime expiry removes warning component");
    require(runtime.dispatch(QStringLiteral("spatial_ui.status"), {}).value.value(QStringLiteral("notification_count")).toInt() == 0,
            "runtime expiry removes warning notification");

    const QString warningId = QStringLiteral("27342995-e416-410a-adf1-625e8a56152e");
    const auto warning = runtime.dispatch(
        QStringLiteral("spatial_ui.notification.create"),
        {{QStringLiteral("schema_version"), QStringLiteral("1.0")},
         {QStringLiteral("notification_id"), warningId}, {QStringLiteral("title"), QStringLiteral("Connection")},
         {QStringLiteral("message"), QStringLiteral("Projection target signal is weak")},
         {QStringLiteral("severity"), QStringLiteral("WARNING")}, {QStringLiteral("privacy_level"), QStringLiteral("PUBLIC")},
         {QStringLiteral("display_target"), QStringLiteral("PHONE")}, {QStringLiteral("timeout_ms"), 5000},
         {QStringLiteral("requires_action"), false}, {QStringLiteral("actions"), QJsonArray {}},
         {QStringLiteral("created_at"), QStringLiteral("2099-07-15T12:00:00.000Z")},
         {QStringLiteral("expires_at"), QStringLiteral("2099-07-15T12:00:05.000Z")}});
    require(warning.ok, "warning notification created");
    require(warning.value.value(QStringLiteral("notification_count")).toInt() == 1, "notification count returned");
    require(runtime.status().value(QStringLiteral("focus_component_id")).toString() == componentId, "warning preserves active focus");
    const auto componentValues = runtime.dispatch(QStringLiteral("spatial_ui.components"), {})
                                     .value.value(QStringLiteral("components")).toArray();
    bool notificationContentExposed = false;
    for (const auto &componentValue : componentValues) {
        const auto value = componentValue.toObject();
        const auto content = value.value(QStringLiteral("content")).toObject();
        if (value.value(QStringLiteral("component_id")).toString() == warningId) {
            notificationContentExposed = content.value(QStringLiteral("title")).toString() == QStringLiteral("Connection")
                && content.value(QStringLiteral("message")).toString() == QStringLiteral("Projection target signal is weak")
                && content.value(QStringLiteral("severity")).toString() == QStringLiteral("WARNING")
                && content.value(QStringLiteral("actions")).toArray().isEmpty();
        }
    }
    require(notificationContentExposed, "component tree exposes notification content");
    const auto cleared = runtime.dispatch(QStringLiteral("spatial_ui.notification.clear"),
                                          {{QStringLiteral("notification_id"), warningId}});
    require(cleared.ok && cleared.value.value(QStringLiteral("notification_count")).toInt() == 0,
            "notification cleared with component state");

    const QString criticalId = QStringLiteral("9c3bff7e-087a-4872-a52c-8ff3629de1be");
    const auto critical = runtime.dispatch(
        QStringLiteral("spatial_ui.notification.create"),
        {{QStringLiteral("schema_version"), QStringLiteral("1.0")},
         {QStringLiteral("notification_id"), criticalId}, {QStringLiteral("title"), QStringLiteral("Privacy confirmation")},
         {QStringLiteral("message"), QStringLiteral("Projection authorization requires confirmation")},
         {QStringLiteral("severity"), QStringLiteral("CRITICAL")}, {QStringLiteral("privacy_level"), QStringLiteral("PRIVATE_SCREEN_ONLY")},
         {QStringLiteral("display_target"), QStringLiteral("PHONE")}, {QStringLiteral("timeout_ms"), 0},
         {QStringLiteral("requires_action"), true}, {QStringLiteral("actions"), QJsonArray {QStringLiteral("Confirm")}},
         {QStringLiteral("created_at"), QStringLiteral("2026-07-15T12:00:00.000Z")},
         {QStringLiteral("expires_at"), QJsonValue {QJsonValue::Null}}});
    require(critical.ok && runtime.status().value(QStringLiteral("focus_component_id")).toString() == criticalId,
            "critical notification preempts focus");
    const QString secondCriticalId = QStringLiteral("1e77d97e-75bc-4a26-ac9b-89b4e9ca31c5");
    QJsonObject secondCriticalParams {{QStringLiteral("schema_version"), QStringLiteral("1.0")},
         {QStringLiteral("notification_id"), secondCriticalId}, {QStringLiteral("title"), QStringLiteral("System protection")},
         {QStringLiteral("message"), QStringLiteral("A second critical action is pending")},
         {QStringLiteral("severity"), QStringLiteral("CRITICAL")}, {QStringLiteral("privacy_level"), QStringLiteral("PRIVATE_SCREEN_ONLY")},
         {QStringLiteral("display_target"), QStringLiteral("PHONE")}, {QStringLiteral("timeout_ms"), 0},
         {QStringLiteral("requires_action"), true}, {QStringLiteral("actions"), QJsonArray {QStringLiteral("Acknowledge")}},
         {QStringLiteral("created_at"), QStringLiteral("2026-07-15T12:00:01.000Z")},
         {QStringLiteral("expires_at"), QJsonValue {QJsonValue::Null}}};
    require(runtime.dispatch(QStringLiteral("spatial_ui.notification.create"), secondCriticalParams).ok
                && runtime.status().value(QStringLiteral("focus_component_id")).toString() == secondCriticalId,
            "second critical notification nests focus preemption");
    QJsonObject nestedCriticalAction = event;
    nestedCriticalAction.insert(QStringLiteral("event_id"), QStringLiteral("6d7e5d71-a599-46f1-8421-ec5119b777cb"));
    nestedCriticalAction.insert(QStringLiteral("event_type"), QStringLiteral("AI_ACTION"));
    nestedCriticalAction.insert(QStringLiteral("source_type"), QStringLiteral("AI_INTENT"));
    nestedCriticalAction.insert(QStringLiteral("source_id"), QStringLiteral("notification-action"));
    nestedCriticalAction.insert(QStringLiteral("target_component_id"), criticalId);
    nestedCriticalAction.insert(QStringLiteral("position"), QJsonValue {QJsonValue::Null});
    nestedCriticalAction.insert(QStringLiteral("action"), QStringLiteral("Confirm"));
    const auto coveredCriticalAction = runtime.dispatch(QStringLiteral("spatial_ui.input"), nestedCriticalAction);
    require(!coveredCriticalAction.ok && coveredCriticalAction.errorCode == 5406
                && coveredCriticalAction.value.value(QStringLiteral("target_component_id")).toString() == secondCriticalId,
            "newest critical notification blocks actions on an older critical notification");
    nestedCriticalAction.insert(QStringLiteral("event_id"), QStringLiteral("ef5b775b-cebd-46b0-845c-98d81231c137"));
    nestedCriticalAction.insert(QStringLiteral("target_component_id"), secondCriticalId);
    nestedCriticalAction.insert(QStringLiteral("action"), QStringLiteral("Acknowledge"));
    require(runtime.dispatch(QStringLiteral("spatial_ui.input"), nestedCriticalAction).ok
                && runtime.status().value(QStringLiteral("focus_component_id")).toString() == criticalId,
            "newest critical action is reachable and restores previous critical focus");
    QJsonObject safetyEvent = event;
    safetyEvent.insert(QStringLiteral("event_id"), QStringLiteral("536dff38-969d-4c4b-942f-b1962ee17fa6"));
    safetyEvent.insert(QStringLiteral("position"), QJsonObject {{QStringLiteral("coordinate_system"), QStringLiteral("PHONE_VIEW")},
                                                                {QStringLiteral("x"), 40.0}, {QStringLiteral("y"), 40.0}});
    const auto blocked = runtime.dispatch(QStringLiteral("spatial_ui.input"), safetyEvent);
    require(!blocked.ok && blocked.errorCode == 5406 && blocked.value.value(QStringLiteral("blocked")).toBool(),
            "critical notification blocks ordinary input");
    QJsonObject criticalAction = safetyEvent;
    criticalAction.insert(QStringLiteral("event_id"), QStringLiteral("83d4f429-e674-4a33-8dbd-9d43b8444870"));
    criticalAction.insert(QStringLiteral("event_type"), QStringLiteral("AI_ACTION"));
    criticalAction.insert(QStringLiteral("source_type"), QStringLiteral("AI_INTENT"));
    criticalAction.insert(QStringLiteral("source_id"), QStringLiteral("notification-action"));
    criticalAction.insert(QStringLiteral("target_component_id"), criticalId);
    criticalAction.insert(QStringLiteral("position"), QJsonValue {QJsonValue::Null});
    criticalAction.insert(QStringLiteral("action"), QStringLiteral("Cancel"));
    const auto undeclaredAction = runtime.dispatch(QStringLiteral("spatial_ui.input"), criticalAction);
    require(!undeclaredAction.ok && undeclaredAction.errorCode == 5402
                && runtime.status().value(QStringLiteral("notification_count")).toInt() == 1,
            "undeclared critical notification action is rejected without clearing notification");
    criticalAction.insert(QStringLiteral("event_id"), QStringLiteral("9d28ea4d-9266-4a35-94f5-385ffda1b978"));
    criticalAction.insert(QStringLiteral("action"), QStringLiteral("Confirm"));
    require(runtime.dispatch(QStringLiteral("spatial_ui.input"), criticalAction).ok,
            "critical notification accepts its own action input");
    require(runtime.status().value(QStringLiteral("notification_count")).toInt() == 0,
            "critical action removes notification");
    const auto componentsAfterCriticalAction = runtime.dispatch(QStringLiteral("spatial_ui.components"), {})
                                                   .value.value(QStringLiteral("components")).toArray();
    bool criticalComponentRetained = false;
    for (const auto &componentValue : componentsAfterCriticalAction) {
        criticalComponentRetained = criticalComponentRetained
            || componentValue.toObject().value(QStringLiteral("component_id")).toString() == criticalId;
    }
    require(!criticalComponentRetained, "critical action removes notification component");
    require(runtime.status().value(QStringLiteral("focus_component_id")).toString() == componentId,
            "focus restored after critical notification action");

    const QString invalidStatePath = temporary.filePath(QStringLiteral("state-path-is-directory"));
    require(QDir {}.mkpath(invalidStatePath), "invalid state path fixture created");
    SpatialUIRuntime failingAutosaveRuntime {invalidStatePath};
    const QString failingComponentId = QStringLiteral("8903e132-b12d-4a1a-8a1c-bac828d439cc");
    const QString failingWindowId = QStringLiteral("07c7772d-d867-4b6e-bb31-3db738bd1cc4");
    require(failingAutosaveRuntime.dispatch(
                QStringLiteral("spatial_ui.component.create"), component(failingComponentId, 10.0, 10.0, 100.0, 100.0)).ok,
            "autosave failure component created");
    require(failingAutosaveRuntime.dispatch(
                QStringLiteral("spatial_ui.window.open"),
                {{QStringLiteral("window_id"), failingWindowId}, {QStringLiteral("component_id"), failingComponentId},
                 {QStringLiteral("bounds"), QJsonObject {{QStringLiteral("x"), 10.0}, {QStringLiteral("y"), 10.0},
                                                          {QStringLiteral("width"), 100.0}, {QStringLiteral("height"), 100.0}}},
                 {QStringLiteral("display_target"), QStringLiteral("PHONE")},
                 {QStringLiteral("projection_target"), QJsonValue {QJsonValue::Null}}}).ok,
            "autosave failure window opened");
    const auto failedAutosave = failingAutosaveRuntime.dispatch(
        QStringLiteral("spatial_ui.window.move"),
        {{QStringLiteral("window_id"), failingWindowId}, {QStringLiteral("x"), 40.0}, {QStringLiteral("y"), 50.0}});
    require(!failedAutosave.ok && failedAutosave.errorCode == 5801, "autosave failure returns state save error");
    require(!failedAutosave.value.contains(QStringLiteral("interaction_completed")),
            "autosave failure does not report interaction completion");

    const auto defaultLayout = runtime.dispatch(QStringLiteral("spatial_ui.layout.reset"), {});
    require(defaultLayout.ok && defaultLayout.value.value(QStringLiteral("mode")).toString() == QStringLiteral("STACK"),
            "layout reset applies default stack");

    const QString taskId = QStringLiteral("5c4402e1-99e7-481a-a94f-61cd07b69d83");
    require(runtime.dispatch(QStringLiteral("spatial_ui.task.create"), task(taskId)).ok, "task surface created before reset");
    require(runtime.dispatch(QStringLiteral("spatial_ui.component.remove"), {{QStringLiteral("component_id"), taskId}}).ok,
            "task component removal clears task surface");
    require(runtime.dispatch(QStringLiteral("spatial_ui.task.create"), task(taskId)).ok,
            "task identifier can be recreated after component removal");
    QJsonObject taskComponent;
    for (const auto &value : runtime.dispatch(QStringLiteral("spatial_ui.components"), {})
                                 .value.value(QStringLiteral("components")).toArray()) {
        if (value.toObject().value(QStringLiteral("component_id")).toString() == taskId) taskComponent = value.toObject();
    }
    const auto taskContent = taskComponent.value(QStringLiteral("content")).toObject();
    require(taskComponent.value(QStringLiteral("component_id")).toString() == taskId
                && taskContent.value(QStringLiteral("title")).toString() == QStringLiteral("Fixture task")
                && taskContent.value(QStringLiteral("summary")).toString() == QStringLiteral("Task lifecycle fixture")
                && taskContent.value(QStringLiteral("state")).toString() == QStringLiteral("CREATED")
                && taskContent.value(QStringLiteral("confidence")).toDouble() == 1.0
                && taskContent.value(QStringLiteral("execution_strategy")).toString() == QStringLiteral("fixture_adapter"),
            "component tree exposes task surface content");

    const QString confirmationTaskId = QStringLiteral("ce9bce8f-a232-4ddf-ad65-ab39d78fcfc5");
    auto confirmationTask = task(confirmationTaskId);
    confirmationTask.insert(QStringLiteral("execution_strategy"), QStringLiteral("REQUIRE_CONFIRMATION"));
    require(runtime.dispatch(QStringLiteral("spatial_ui.task.create"), confirmationTask).ok,
            "confirmation task surface created");
    QJsonObject taskAction = event;
    taskAction.insert(QStringLiteral("event_id"), QStringLiteral("aa65f807-c299-45be-a4cb-a0dc5a384369"));
    taskAction.insert(QStringLiteral("event_type"), QStringLiteral("AI_ACTION"));
    taskAction.insert(QStringLiteral("source_type"), QStringLiteral("SYSTEM"));
    taskAction.insert(QStringLiteral("source_id"), QStringLiteral("astra-shell"));
    taskAction.insert(QStringLiteral("target_component_id"), confirmationTaskId);
    taskAction.insert(QStringLiteral("position"), QJsonValue {QJsonValue::Null});
    taskAction.insert(QStringLiteral("action"), QStringLiteral("confirm"));
    const auto confirmedTask = runtime.dispatch(QStringLiteral("spatial_ui.input"), taskAction);
    require(confirmedTask.ok && confirmedTask.value.value(QStringLiteral("task_state")).toString() == QStringLiteral("READY"),
            "task confirmation action is routed into the task surface");
    taskAction.insert(QStringLiteral("event_id"), QStringLiteral("394a8b51-6318-42d1-880f-0f1309852657"));
    taskAction.insert(QStringLiteral("action"), QStringLiteral("cancel"));
    const auto cancelledTask = runtime.dispatch(QStringLiteral("spatial_ui.input"), taskAction);
    require(cancelledTask.ok && cancelledTask.value.value(QStringLiteral("task_state")).toString() == QStringLiteral("CANCELLED"),
            "task cancellation action is routed into the task surface");

    const auto reset = runtime.dispatch(QStringLiteral("spatial_ui.reset"), {});
    require(reset.ok && reset.value.value(QStringLiteral("projection_safe")).toBool(), "runtime reset to safe state");
    const auto resetStatus = runtime.status();
    require(resetStatus.value(QStringLiteral("component_count")).toInt() == 0, "reset removes components");
    require(resetStatus.value(QStringLiteral("window_count")).toInt() == 0, "reset removes windows");
    require(resetStatus.value(QStringLiteral("notification_count")).toInt() == 0, "reset removes notifications");
    require(resetStatus.value(QStringLiteral("focus_component_id")).toString().isEmpty(), "reset releases focus");
    require(runtime.dispatch(QStringLiteral("spatial_ui.task.create"), task(taskId)).ok,
            "task identifier can be recreated after runtime reset");
    require(runtime.dispatch(QStringLiteral("spatial_ui.component.remove"), {{QStringLiteral("component_id"), taskId}}).ok,
            "recreated task removed after reset test");

    require(runtime.dispatch(QStringLiteral("spatial_ui.component.create"), component(componentId, 20.0, 30.0, 200.0, 100.0)).ok,
            "keyboard target recreated");
    QJsonObject keyEvent = event;
    keyEvent.insert(QStringLiteral("event_id"), QStringLiteral("19b34774-6f16-419a-a96c-463531f303b2"));
    keyEvent.insert(QStringLiteral("event_type"), QStringLiteral("KEY_PRESS"));
    keyEvent.insert(QStringLiteral("source_type"), QStringLiteral("KEYBOARD"));
    keyEvent.insert(QStringLiteral("source_id"), QStringLiteral("primary-keyboard"));
    keyEvent.insert(QStringLiteral("target_component_id"), componentId);
    keyEvent.insert(QStringLiteral("position"), QJsonValue {QJsonValue::Null});
    QJsonObject focusEvent = keyEvent;
    focusEvent.insert(QStringLiteral("event_id"), QStringLiteral("00742d99-0353-49f8-ab43-113a305f4e89"));
    focusEvent.insert(QStringLiteral("event_type"), QStringLiteral("SYSTEM_FOCUS"));
    focusEvent.insert(QStringLiteral("source_type"), QStringLiteral("MOUSE"));
    focusEvent.insert(QStringLiteral("source_id"), QStringLiteral("primary-mouse"));
    const auto routedFocus = runtime.dispatch(QStringLiteral("spatial_ui.input"), focusEvent);
    require(routedFocus.ok && runtime.status().value(QStringLiteral("focus_component_id")).toString() == componentId,
            "unified Shell focus input reaches the focus manager");
    const auto keyRoute = runtime.dispatch(QStringLiteral("spatial_ui.input"), keyEvent);
    require(keyRoute.ok && keyRoute.value.value(QStringLiteral("target_component_id")).toString() == componentId,
            "targeted keyboard input routed without position");
    require(runtime.dispatch(QStringLiteral("spatial_ui.components"), {}).value.value(QStringLiteral("components")).toArray().size() == 1,
            "component state query");
    require(runtime.dispatch(QStringLiteral("spatial_ui.windows"), {}).value.value(QStringLiteral("windows")).toArray().isEmpty(),
            "window state query");
    require(runtime.dispatch(QStringLiteral("spatial_ui.focus.status"), {}).ok, "focus state query");
    require(runtime.dispatch(QStringLiteral("spatial_ui.layout.status"), {}).value.value(QStringLiteral("mode")).toString() == QStringLiteral("STACK"),
            "layout state query");
    require(runtime.dispatch(QStringLiteral("spatial_ui.metrics"), {}).value.value(QStringLiteral("component_count")).toInt() == 1,
            "metrics state query");
}
