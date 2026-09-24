#include "astra/spatial_ui/service/SpatialUIRuntime.h"

#include <QTemporaryDir>
#include <QJsonArray>
#include <QJsonDocument>
#include <QFile>
#include <QtGlobal>

using astra::spatial_ui::service::SpatialUIRuntime;

namespace {

void require(bool condition)
{
    if (!condition) qFatal("P5 runtime state restoration requirement failed");
}

} // namespace

int main()
{
    QTemporaryDir temporary;
    require(temporary.isValid());
    const QString statePath = temporary.filePath(QStringLiteral("ui-state.json"));
    const QString componentId = QStringLiteral("7abf7b1e-dc4f-4b7c-96cc-c3dd628e1ca9");
    const QString windowId = QStringLiteral("c38c0e21-96f1-4975-8cf3-fbfaa58c7cda");

    astra::spatial_ui::service::SpatialUIRuntimeOptions options;
    options.maximumFixtureDisclosure = astra::common::PrivacyLevel::PrivateScreenOnly;
    options.publicFixtureSubjectId = componentId;
    SpatialUIRuntime first {statePath, {}, options};
    auto result = first.dispatch(QStringLiteral("spatial_ui.component.create"),
                                 {{QStringLiteral("component_id"), componentId}, {QStringLiteral("component_type"), QStringLiteral("TASK_CARD")},
                                  {QStringLiteral("privacy_level"), QStringLiteral("PUBLIC")},
                                  {QStringLiteral("accessibility_label"), QStringLiteral("Restorable task")},
                                  {QStringLiteral("bounds"), QJsonObject {{QStringLiteral("x"), 40.0}, {QStringLiteral("y"), 50.0},
                                                                           {QStringLiteral("width"), 360.0}, {QStringLiteral("height"), 180.0}}},
                                  {QStringLiteral("display_target"), QStringLiteral("PHONE")}});
    require(result.ok);
    result = first.dispatch(QStringLiteral("spatial_ui.window.open"),
                            {{QStringLiteral("window_id"), windowId}, {QStringLiteral("component_id"), componentId},
                             {QStringLiteral("bounds"), QJsonObject {{QStringLiteral("x"), 40.0}, {QStringLiteral("y"), 50.0},
                                                                      {QStringLiteral("width"), 360.0}, {QStringLiteral("height"), 180.0}}},
                             {QStringLiteral("display_target"), QStringLiteral("PHONE")}, {QStringLiteral("projection_target"), QJsonValue::Null}});
    require(result.ok);
    require(first.dispatch(QStringLiteral("spatial_ui.focus"),
                           {{QStringLiteral("component_id"), componentId}, {QStringLiteral("reason"), QStringLiteral("restore fixture")}}).ok);
    require(first.dispatch(QStringLiteral("spatial_ui.state.save"), {}).ok);

    QFile stateFile {statePath};
    require(stateFile.open(QIODevice::ReadOnly));
    QJsonObject persisted = QJsonDocument::fromJson(stateFile.readAll()).object();
    stateFile.close();
    require(!persisted.value(QStringLiteral("components")).toArray().at(0).toObject().contains(QStringLiteral("privacy_level")));

    SpatialUIRuntime restored {statePath, {}, options};
    result = restored.dispatch(QStringLiteral("spatial_ui.state.load"), {});
    require(result.ok);
    const QJsonObject status = restored.status();
    require(status.value(QStringLiteral("component_count")).toInt() == 1);
    require(status.value(QStringLiteral("window_count")).toInt() == 1);
    require(status.value(QStringLiteral("focus_component_id")).toString() == componentId);
    const auto components = restored.dispatch(QStringLiteral("spatial_ui.components"), {});
    require(components.ok);
    require(components.value.value(QStringLiteral("components")).toArray().at(0).toObject()
                .value(QStringLiteral("privacy_level")).toString() == QStringLiteral("PRIVATE_SCREEN_ONLY"));

    // A closed window is not persisted, and removing a component removes its windows,
    // so saved state always stays restorable.
    require(restored.dispatch(QStringLiteral("spatial_ui.window.close"), {{QStringLiteral("window_id"), windowId}}).ok);
    require(restored.dispatch(QStringLiteral("spatial_ui.state.save"), {}).ok);
    require(stateFile.open(QIODevice::ReadOnly));
    persisted = QJsonDocument::fromJson(stateFile.readAll()).object();
    stateFile.close();
    require(persisted.value(QStringLiteral("windows")).toArray().isEmpty());
    require(restored.dispatch(QStringLiteral("spatial_ui.component.remove"), {{QStringLiteral("component_id"), componentId}}).ok);
    require(restored.status().value(QStringLiteral("window_count")).toInt() == 0);
    require(restored.dispatch(QStringLiteral("spatial_ui.state.save"), {}).ok);
    SpatialUIRuntime afterRemoval {statePath, {}, options};
    require(afterRemoval.dispatch(QStringLiteral("spatial_ui.state.load"), {}).ok);

    // A failed restore rolls back instead of leaving a half-populated runtime.
    persisted.insert(QStringLiteral("components"), QJsonArray {QJsonObject {
        {QStringLiteral("component_id"), componentId}, {QStringLiteral("component_type"), QStringLiteral("TASK_CARD")},
        {QStringLiteral("bounds"), QJsonObject {{QStringLiteral("x"), 40.0}, {QStringLiteral("y"), 50.0},
                                                 {QStringLiteral("width"), 360.0}, {QStringLiteral("height"), 180.0}}},
        {QStringLiteral("z_order"), 0}, {QStringLiteral("visible"), true}, {QStringLiteral("focusable"), true},
        {QStringLiteral("interactive"), true}, {QStringLiteral("display_target"), QStringLiteral("PHONE")}}});
    persisted.insert(QStringLiteral("windows"), QJsonArray {QJsonObject {
        {QStringLiteral("window_id"), windowId}, {QStringLiteral("component_id"), QStringLiteral("0b9f7d47-8c5e-4d7e-9c1a-3c1b6f1d2e10")},
        {QStringLiteral("bounds"), QJsonObject {{QStringLiteral("x"), 40.0}, {QStringLiteral("y"), 50.0},
                                                 {QStringLiteral("width"), 360.0}, {QStringLiteral("height"), 180.0}}},
        {QStringLiteral("visible"), true}, {QStringLiteral("display_target"), QStringLiteral("PHONE")},
        {QStringLiteral("projection_target"), QJsonValue::Null}, {QStringLiteral("anchor_id"), QJsonValue::Null},
        {QStringLiteral("focus_scope"), QStringLiteral("workspace")}}});
    persisted.insert(QStringLiteral("focus_restore_component_id"), QString {});
    require(stateFile.open(QIODevice::WriteOnly | QIODevice::Truncate));
    stateFile.write(QJsonDocument {persisted}.toJson());
    stateFile.close();
    SpatialUIRuntime rollback {statePath, {}, options};
    require(rollback.dispatch(QStringLiteral("spatial_ui.state.load"), {}).errorCode == 5805);
    require(rollback.status().value(QStringLiteral("component_count")).toInt() == 0);
    require(rollback.dispatch(QStringLiteral("spatial_ui.state.load"), {}).errorCode == 5805);

    // Removing a notification through the component API also releases the notification record.
    SpatialUIRuntime notifications {temporary.filePath(QStringLiteral("notification-state.json")), {}, options};
    const QString notificationId = QStringLiteral("3f0f6f2a-6a3c-4f33-9d0e-8a3f6e4b1c22");
    const QJsonObject notification {
        {QStringLiteral("schema_version"), QStringLiteral("1.0")}, {QStringLiteral("notification_id"), notificationId},
        {QStringLiteral("title"), QStringLiteral("Connection")}, {QStringLiteral("message"), QStringLiteral("Signal is weak")},
        {QStringLiteral("severity"), QStringLiteral("CRITICAL")}, {QStringLiteral("privacy_level"), QStringLiteral("PRIVATE_SCREEN_ONLY")},
        {QStringLiteral("display_target"), QStringLiteral("PHONE")}, {QStringLiteral("timeout_ms"), 0},
        {QStringLiteral("requires_action"), false}, {QStringLiteral("actions"), QJsonArray {}},
        {QStringLiteral("created_at"), QStringLiteral("2099-07-15T12:00:00.000Z")}, {QStringLiteral("expires_at"), QJsonValue::Null}};
    require(notifications.dispatch(QStringLiteral("spatial_ui.notification.create"), notification).ok);
    require(notifications.status().value(QStringLiteral("focus_component_id")).toString() == notificationId);
    require(notifications.dispatch(QStringLiteral("spatial_ui.component.remove"), {{QStringLiteral("component_id"), notificationId}}).ok);
    const QJsonObject afterNotification = notifications.status();
    require(afterNotification.value(QStringLiteral("notification_count")).toInt() == 0);
    require(afterNotification.value(QStringLiteral("component_count")).toInt() == 0);
    require(afterNotification.value(QStringLiteral("focus_component_id")).toString().isEmpty());
    require(notifications.dispatch(QStringLiteral("spatial_ui.notification.create"), notification).ok);
}
