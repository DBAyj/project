#include "astra/spatial_ui/service/ProjectionGateway.h"
#include "astra/spatial_ui/service/SpatialUIRuntime.h"

#include <QTemporaryDir>
#include <QUuid>
#include <QtGlobal>

#include <memory>

using namespace astra::spatial_ui::service;

namespace {

void require(bool condition)
{
    if (!condition) qFatal("P5 to P4 projection boundary requirement failed");
}

class RecordingProjectionGateway final : public ProjectionGateway {
public:
    ProjectionGatewayResult submit(const QVector<astra::render::ProjectionLayer> &layers,
                                   const QString &traceId,
                                   const QString &requestId) override
    {
        ++submitCount;
        lastLayers = layers;
        lastTraceId = traceId;
        lastRequestId = requestId;
        return failSubmit ? ProjectionGatewayResult {{false, 5904, QStringLiteral("fixture P4 failure")}, 0}
                          : ProjectionGatewayResult {{true, 0, {}}, static_cast<quint64>(submitCount)};
    }

    astra::ui::OperationResult clear(const QString &reason, const QString &, const QString &) override
    {
        ++clearCount;
        lastClearReason = reason;
        return {true, 0, {}};
    }

    bool failSubmit {false};
    int submitCount {0};
    int clearCount {0};
    QVector<astra::render::ProjectionLayer> lastLayers;
    QString lastTraceId;
    QString lastRequestId;
    QString lastClearReason;
};

QJsonObject component(const QString &id, const QString &privacy)
{
    return {{QStringLiteral("component_id"), id}, {QStringLiteral("component_type"), QStringLiteral("TASK_CARD")},
            {QStringLiteral("privacy_level"), privacy}, {QStringLiteral("accessibility_label"), QStringLiteral("Public task")},
            {QStringLiteral("bounds"), QJsonObject {{QStringLiteral("x"), 20.0}, {QStringLiteral("y"), 30.0},
                                                     {QStringLiteral("width"), 320.0}, {QStringLiteral("height"), 180.0}}},
            {QStringLiteral("display_target"), QStringLiteral("BOTH")}};
}

} // namespace

int main()
{
    QTemporaryDir temporary;
    require(temporary.isValid());
    auto gateway = std::make_shared<RecordingProjectionGateway>();
    SpatialUIRuntimeOptions options;
    options.projectionTargetEnabled = true;
    options.configurationStatus = QStringLiteral("VALID");
    options.configurationWarning.clear();
    options.publicFixtureSubjectId = QStringLiteral("7abf7b1e-dc4f-4b7c-96cc-c3dd628e1ca9");
    options.policyBindingKey = QStringLiteral("fixture-policy-binding-key");
    SpatialUIRuntime runtime {temporary.filePath(QStringLiteral("state.json")), gateway, options};

    const QString publicId = QStringLiteral("7abf7b1e-dc4f-4b7c-96cc-c3dd628e1ca9");
    const QString privateId = QStringLiteral("c38c0e21-96f1-4975-8cf3-fbfaa58c7cda");
    const QString traceId = QStringLiteral("98b998ba-e9e5-49f7-af4d-ce85d91a685c");
    const QString requestId = QStringLiteral("acfe1ec3-18f5-4489-88bc-582c2a2b6274");

    auto result = runtime.dispatch(QStringLiteral("spatial_ui.component.create"), component(publicId, QStringLiteral("PUBLIC")), traceId, requestId);
    require(result.ok);
    require(result.value.value(QStringLiteral("projection_layer_generated")).toBool());
    require(gateway->submitCount == 1 && gateway->lastLayers.size() == 1);
    require(gateway->lastLayers.first().id == publicId);
    require(gateway->lastLayers.first().privacyLevel == astra::common::PrivacyLevel::Public);
    require(!QUuid {gateway->lastLayers.first().policyDecisionId}.isNull());
    require(gateway->lastLayers.first().bounds == QRectF(20.0, 30.0, 320.0, 180.0));
    require(gateway->lastTraceId == traceId && gateway->lastRequestId == requestId);

    result = runtime.dispatch(QStringLiteral("spatial_ui.component.create"), component(privateId, QStringLiteral("PRIVATE_SCREEN_ONLY")), traceId, requestId);
    require(result.ok);
    require(!result.value.value(QStringLiteral("projection_layer_generated")).toBool());
    require(gateway->lastLayers.size() == 1 && gateway->lastLayers.first().id == publicId);

    result = runtime.dispatch(QStringLiteral("spatial_ui.target.lost"), {}, traceId, requestId);
    require(result.ok);
    require(gateway->clearCount == 1 && gateway->lastClearReason == QStringLiteral("TARGET_LOST"));
    require(runtime.status().value(QStringLiteral("projection_safe")).toBool());

    auto failingGateway = std::make_shared<RecordingProjectionGateway>();
    failingGateway->failSubmit = true;
    SpatialUIRuntime failingRuntime {temporary.filePath(QStringLiteral("failed-state.json")), failingGateway, options};
    result = failingRuntime.dispatch(QStringLiteral("spatial_ui.component.create"), component(publicId, QStringLiteral("PUBLIC")), traceId, requestId);
    require(!result.ok && result.errorCode == 5904);
    require(failingRuntime.status().value(QStringLiteral("projection_safe")).toBool());
}
