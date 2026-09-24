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
}
