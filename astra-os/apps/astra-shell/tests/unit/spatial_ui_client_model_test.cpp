#include "clients/SpatialUIServiceClient.h"
#include "models/SpatialUIStateModel.h"

#include <QJsonArray>
#include <QJsonObject>
#include <QtGlobal>

namespace {
void require(bool condition)
{
    if (!condition) qFatal("P5 Shell spatial UI integration requirement failed");
}
}

int main()
{
    const QJsonObject response {{QStringLiteral("jsonrpc"), QStringLiteral("2.0")}, {QStringLiteral("id"), QStringLiteral("request-1")},
                                {QStringLiteral("result"), QJsonObject {{QStringLiteral("status"), QStringLiteral("READY")},
                                                                         {QStringLiteral("component_count"), 4}, {QStringLiteral("window_count"), 2},
                                                                         {QStringLiteral("focus_component_id"), QStringLiteral("7abf7b1e-dc4f-4b7c-96cc-c3dd628e1ca9")},
                                                                         {QStringLiteral("projection_safe"), true},
                                                                         {QStringLiteral("reduce_motion"), true},
                                                                         {QStringLiteral("high_contrast"), true},
                                                                         {QStringLiteral("p4_release_status"), QStringLiteral("P4_RELEASE_BASELINE_FINAL")}}}};
    const auto decoded = astra::shell::SpatialUIServiceClient::decodeResponse(response);
    require(decoded.ok && decoded.result.value(QStringLiteral("component_count")).toInt() == 4);
    astra::shell::SpatialUIStateModel model;
    model.applyStatus(decoded.result);
    require(model.status() == QStringLiteral("READY"));
    require(model.componentCount() == 4 && model.windowCount() == 2);
    require(model.projectionSafe());
    require(model.reduceMotion() && model.highContrast());
    require(model.p4ReleaseStatus() == QStringLiteral("P4_RELEASE_BASELINE_FINAL"));

    const QJsonArray components {
        QJsonObject {{QStringLiteral("component_id"), QStringLiteral("phone-private")},
                     {QStringLiteral("component_type"), QStringLiteral("TASK_CARD")},
                     {QStringLiteral("display_target"), QStringLiteral("PHONE")},
                     {QStringLiteral("privacy_level"), QStringLiteral("PRIVATE_SCREEN_ONLY")}},
        QJsonObject {{QStringLiteral("component_id"), QStringLiteral("shared-public")},
                     {QStringLiteral("component_type"), QStringLiteral("SYSTEM_PANEL")},
                     {QStringLiteral("display_target"), QStringLiteral("BOTH")},
                     {QStringLiteral("privacy_level"), QStringLiteral("PUBLIC")}},
        QJsonObject {{QStringLiteral("component_id"), QStringLiteral("projection-public")},
                     {QStringLiteral("component_type"), QStringLiteral("NOTIFICATION")},
                     {QStringLiteral("display_target"), QStringLiteral("PROJECTION")},
                     {QStringLiteral("privacy_level"), QStringLiteral("PUBLIC")}},
        QJsonObject {{QStringLiteral("component_id"), QStringLiteral("projection-private")},
                     {QStringLiteral("component_type"), QStringLiteral("LABEL")},
                     {QStringLiteral("display_target"), QStringLiteral("PROJECTION")},
                     {QStringLiteral("privacy_level"), QStringLiteral("NO_PROJECTION")}},
    };
    model.applyComponents(QJsonObject {{QStringLiteral("components"), components}, {QStringLiteral("component_count"), components.size()}});
    require(model.components().size() == 4);

    const auto phoneComponents = model.componentsForDisplay(QStringLiteral("PHONE"));
    require(phoneComponents.size() == 2);
    require(phoneComponents.at(0).toMap().value(QStringLiteral("component_id")).toString() == QStringLiteral("phone-private"));
    require(phoneComponents.at(1).toMap().value(QStringLiteral("component_id")).toString() == QStringLiteral("shared-public"));

    const auto projectionComponents = model.componentsForDisplay(QStringLiteral("PROJECTION"));
    require(projectionComponents.size() == 2);
    require(projectionComponents.at(0).toMap().value(QStringLiteral("component_id")).toString() == QStringLiteral("shared-public"));
    require(projectionComponents.at(1).toMap().value(QStringLiteral("component_id")).toString() == QStringLiteral("projection-public"));

    const QJsonArray hierarchyComponents {
        QJsonObject {{QStringLiteral("component_id"), QStringLiteral("child")},
                     {QStringLiteral("parent_id"), QStringLiteral("parent")},
                     {QStringLiteral("children"), QJsonArray {}},
                     {QStringLiteral("display_target"), QStringLiteral("PHONE")},
                     {QStringLiteral("privacy_level"), QStringLiteral("PUBLIC")}},
        QJsonObject {{QStringLiteral("component_id"), QStringLiteral("parent")},
                     {QStringLiteral("parent_id"), QJsonValue {QJsonValue::Null}},
                     {QStringLiteral("children"), QJsonArray {QStringLiteral("child")}},
                     {QStringLiteral("display_target"), QStringLiteral("PHONE")},
                     {QStringLiteral("privacy_level"), QStringLiteral("PUBLIC")}},
    };
    model.applyComponents(QJsonObject {{QStringLiteral("components"), hierarchyComponents},
                                       {QStringLiteral("component_count"), hierarchyComponents.size()}});
    const auto hierarchy = model.componentsForDisplay(QStringLiteral("PHONE"));
    require(hierarchy.size() == 1);
    require(hierarchy.at(0).toMap().value(QStringLiteral("component_id")).toString() == QStringLiteral("parent"));
    require(hierarchy.at(0).toMap().value(QStringLiteral("hierarchy_depth")).toInt() == 0);
    const auto childComponents = hierarchy.at(0).toMap().value(QStringLiteral("child_components")).toList();
    require(childComponents.size() == 1);
    require(childComponents.at(0).toMap().value(QStringLiteral("component_id")).toString() == QStringLiteral("child"));
    require(childComponents.at(0).toMap().value(QStringLiteral("hierarchy_depth")).toInt() == 1);

    const QJsonObject error {{QStringLiteral("error"), QJsonObject {{QStringLiteral("code"), 5002}, {QStringLiteral("message"), QStringLiteral("denied")}}}};
    const auto rejected = astra::shell::SpatialUIServiceClient::decodeResponse(error);
    require(!rejected.ok && rejected.errorCode == 5002);
}
