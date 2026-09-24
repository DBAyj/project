#include "astra/projection/service/ProjectionService.h"
#include "astra/policy/ProjectionPolicyService.h"
#include "astra/common/CapabilityToken.h"

#include <QJsonObject>
#include <QJsonArray>
#include <QCryptographicHash>
#include <QGuiApplication>
#include <QImage>

namespace {

void require(bool condition, const char *message)
{
    if (!condition) qFatal("P4 projection service requirement failed: %s", message);
}

QJsonObject request(const QString &method, const QJsonObject &params, const QString &token)
{
    const QString capability = astra::common::projectionCapabilityForMethod(method);
    return {{QStringLiteral("jsonrpc"), QStringLiteral("2.0")},
            {QStringLiteral("id"), QStringLiteral("test-request")},
            {QStringLiteral("method"), method},
            {QStringLiteral("params"), params},
            {QStringLiteral("security_context"), QJsonObject {{QStringLiteral("capability_token"),
                                                                   astra::common::scopedCapabilityToken(token, capability)}}}};
}

QJsonObject requestWithCapability(const QString &method, const QJsonObject &params, const QString &capabilityToken)
{
    return {{QStringLiteral("jsonrpc"), QStringLiteral("2.0")},
            {QStringLiteral("id"), QStringLiteral("test-request")},
            {QStringLiteral("method"), method},
            {QStringLiteral("params"), params},
            {QStringLiteral("security_context"), QJsonObject {{QStringLiteral("capability_token"), capabilityToken}}}};
}

QJsonObject commandParameters(const QString &sessionId, const QString &command)
{
    return {{QStringLiteral("session_id"), sessionId}, {QStringLiteral("command"), command}};
}

QJsonObject renderParameters(const QString &sessionId, const QString &privacyLevel)
{
    return {{QStringLiteral("request_id"), QStringLiteral("7abf7b1e-dc4f-4b7c-96cc-c3dd628e1ca9")},
            {QStringLiteral("trace_id"), QStringLiteral("7abf7b1e-dc4f-4b7c-96cc-c3dd628e1ca9")}, {QStringLiteral("session_id"), sessionId},
            {QStringLiteral("output_id"), QStringLiteral("p4-window-projection")}, {QStringLiteral("fixture_id"), QStringLiteral("front-rectangle")},
            {QStringLiteral("layer_ids"), QJsonArray {QStringLiteral("c38c0e21-96f1-4975-8cf3-fbfaa58c7cda")}},
            {QStringLiteral("privacy_level"), privacyLevel}, {QStringLiteral("p3_integration_status"), QStringLiteral("P3_REAL_SPATIAL_INTEGRATION_NOT_VERIFIED")}};
}

QJsonObject layerSubmitParameters(const QString &sessionId, const QString &privacyLevel)
{
    const QString layerId = QStringLiteral("98b998ba-e9e5-49f7-af4d-ce85d91a685c");
    const QString decisionId = astra::policy::ProjectionPolicyService::bindingId(
        layerId, astra::common::PrivacyLevel::Public, true,
        QString::fromLatin1(astra::projection::service::kFixtureCapabilityToken));
    return {{QStringLiteral("request_id"), QStringLiteral("7abf7b1e-dc4f-4b7c-96cc-c3dd628e1ca9")},
            {QStringLiteral("trace_id"), QStringLiteral("c38c0e21-96f1-4975-8cf3-fbfaa58c7cda")},
            {QStringLiteral("session_id"), sessionId},
            {QStringLiteral("output_id"), QStringLiteral("p4-window-projection")},
            {QStringLiteral("fixture_id"), QStringLiteral("front-rectangle")},
            {QStringLiteral("layers"), QJsonArray {QJsonObject {
                 {QStringLiteral("layer_id"), layerId},
                 {QStringLiteral("layer_type"), QStringLiteral("APPLICATION_SURFACE")},
                 {QStringLiteral("privacy_level"), privacyLevel},
                 {QStringLiteral("policy_decision_id"), decisionId},
                 {QStringLiteral("policy_subject_id"), layerId},
                 {QStringLiteral("visible"), true},
                 {QStringLiteral("z_index"), 4},
                 {QStringLiteral("bounds"), QJsonObject {{QStringLiteral("x"), 24}, {QStringLiteral("y"), 32},
                                                         {QStringLiteral("width"), 180}, {QStringLiteral("height"), 72}}},
                 {QStringLiteral("public_label"), QStringLiteral("Public task")}}}},
            {QStringLiteral("p3_integration_status"), QStringLiteral("P3_REAL_SPATIAL_INTEGRATION_NOT_VERIFIED")}};
}

QJsonObject verifiedLayerSubmitParameters(const QString &sessionId)
{
    QJsonObject params = layerSubmitParameters(sessionId, QStringLiteral("PUBLIC"));
    params.insert(QStringLiteral("p3_integration_status"), QStringLiteral("P3_SERVICE_PROJECTION_TARGET_VERIFIED"));
    params.insert(QStringLiteral("spatial_target"), QJsonObject {
        {QStringLiteral("service"), QStringLiteral("astra-spatial-service")},
        {QStringLiteral("input_source"), QStringLiteral("SIMULATION")},
        {QStringLiteral("state"), QStringLiteral("TRACKING")},
        {QStringLiteral("target"), QJsonObject {
             {QStringLiteral("target_id"), QStringLiteral("63f9b938-c56d-40d8-8880-9374e4ba35f2")},
             {QStringLiteral("surface_id"), QStringLiteral("80450f7a-1195-407f-bede-f09d7e88c842")},
             {QStringLiteral("state"), QStringLiteral("CALIBRATED")},
             {QStringLiteral("selected"), true},
             {QStringLiteral("quality"), QStringLiteral("GOOD")},
             {QStringLiteral("privacy_level"), QStringLiteral("PUBLIC")},
             {QStringLiteral("coordinate_system"), QStringLiteral("IMAGE_PIXEL")},
             {QStringLiteral("corners"), QJsonArray {
                  QJsonObject {{QStringLiteral("x"), 10.0}, {QStringLiteral("y"), 10.0}, {QStringLiteral("coordinate_system"), QStringLiteral("IMAGE_PIXEL")}},
                  QJsonObject {{QStringLiteral("x"), 620.0}, {QStringLiteral("y"), 10.0}, {QStringLiteral("coordinate_system"), QStringLiteral("IMAGE_PIXEL")}},
                  QJsonObject {{QStringLiteral("x"), 620.0}, {QStringLiteral("y"), 470.0}, {QStringLiteral("coordinate_system"), QStringLiteral("IMAGE_PIXEL")}},
                  QJsonObject {{QStringLiteral("x"), 10.0}, {QStringLiteral("y"), 470.0}, {QStringLiteral("coordinate_system"), QStringLiteral("IMAGE_PIXEL")}},
             }},
        }},
    });
    return params;
}

} // namespace

int main(int argc, char **argv)
{
    QGuiApplication application {argc, argv};
    astra::projection::service::ProjectionService service;
    const auto denied = service.handleRequest(request(QStringLiteral("projection.session.command"),
                                                      commandParameters(service.sessionId(), QStringLiteral("INITIALIZE")),
                                                      QStringLiteral("invalid")));
    require(denied.contains(QStringLiteral("error")), "invalid capability denied");
    require(denied.value(QStringLiteral("error")).toObject().value(QStringLiteral("code")).toInt() == 5002,
            "invalid capability uses registered denial code");

    const QString token = astra::projection::service::kFixtureCapabilityToken;
    const auto rawTokenDenied = service.handleRequest(requestWithCapability(
        QStringLiteral("projection.session.command"), commandParameters(service.sessionId(), QStringLiteral("STOP")), token));
    require(rawTokenDenied.value(QStringLiteral("error")).toObject().value(QStringLiteral("code")).toInt() == 5002,
            "unscoped raw token cannot control projection");
    const auto readTokenCannotControl = service.handleRequest(requestWithCapability(
        QStringLiteral("projection.session.command"), commandParameters(service.sessionId(), QStringLiteral("STOP")),
        astra::common::scopedCapabilityToken(token, QStringLiteral("projection.output.read"))));
    require(readTokenCannotControl.value(QStringLiteral("error")).toObject().value(QStringLiteral("code")).toInt() == 5002,
            "read-scoped token cannot control projection");
    const auto noFrameStop = service.handleRequest(request(QStringLiteral("projection.session.command"),
                                                          commandParameters(service.sessionId(), QStringLiteral("STOP")), token));
    require(noFrameStop.contains(QStringLiteral("result")), "stop without a frame succeeds");
    require(noFrameStop.value(QStringLiteral("result")).toObject().value(QStringLiteral("state")).toString() == QStringLiteral("IDLE"),
            "stop without a frame leaves the service idle");
    const auto initialized = service.handleRequest(request(QStringLiteral("projection.session.command"),
                                                           commandParameters(service.sessionId(), QStringLiteral("INITIALIZE")), token));
    require(initialized.contains(QStringLiteral("result")), "projection session initializes");
    const auto ready = service.handleRequest(request(QStringLiteral("projection.session.command"),
                                                     commandParameters(service.sessionId(), QStringLiteral("READY")), token));
    require(ready.contains(QStringLiteral("result")), "projection session becomes ready");
    const auto rendered = service.handleRequest(request(
        QStringLiteral("projection.render"),
        renderParameters(service.sessionId(), QStringLiteral("PUBLIC")), token));
    require(rendered.contains(QStringLiteral("result")), "fixture render succeeds");
    require(service.outputHasContent(), "fixture render reaches output");
    const auto frame = service.handleRequest(request(QStringLiteral("projection.output.frame"),
                                                     {{QStringLiteral("output_id"), QStringLiteral("p4-window-projection")}}, token));
    require(frame.contains(QStringLiteral("result")), "rendered output frame is readable");
    const QByteArray encoded = QByteArray::fromBase64(frame.value(QStringLiteral("result")).toObject().value(QStringLiteral("frame_png_base64")).toString().toLatin1());
    QImage renderedFrame;
    require(renderedFrame.loadFromData(encoded, "PNG"), "rendered output is a valid PNG");
    renderedFrame = renderedFrame.convertToFormat(QImage::Format_RGBA8888);
    require(renderedFrame.size() == QSize {256, 144}, "rendered output has the fixture dimensions");
    require(renderedFrame.pixelColor(0, 0) == QColor {240, 240, 240}, "rendered output retains the light checker cell");
    require(renderedFrame.pixelColor(16, 0) == QColor {24, 24, 24}, "rendered output retains the dark checker cell");
    const QByteArray rawPixels {reinterpret_cast<const char *>(renderedFrame.constBits()), static_cast<qsizetype>(renderedFrame.sizeInBytes())};
    require(QCryptographicHash::hash(rawPixels, QCryptographicHash::Sha256).toHex()
                == QByteArrayLiteral("7e8b5046a359e34fd2d0b005959761589c8b577157ebe0ccee33b3f6b88c0e79"),
            "rendered output pixels match the deterministic fixture hash");

    const auto p5Layers = service.handleRequest(request(QStringLiteral("projection.layers.submit"),
                                                        layerSubmitParameters(service.sessionId(), QStringLiteral("PUBLIC")), token));
    require(p5Layers.contains(QStringLiteral("result")), "P5 public layer submit succeeds");
    require(p5Layers.value(QStringLiteral("result")).toObject().value(QStringLiteral("accepted_layer_count")).toInt() == 1,
            "P5 public layer submit accepts one layer");
    const auto p5Frame = service.handleRequest(request(QStringLiteral("projection.output.frame"),
                                                       {{QStringLiteral("output_id"), QStringLiteral("p4-window-projection")}}, token));
    require(p5Frame.value(QStringLiteral("result")).toObject().value(QStringLiteral("source")).toString()
                == QStringLiteral("P5_SPATIAL_UI"),
            "P5 frame records the spatial UI source");
    require(p5Frame.value(QStringLiteral("result")).toObject().value(QStringLiteral("public_labels")).toArray()
                == QJsonArray {QStringLiteral("Public task")},
            "P5 frame exposes only the public layer label");

    const auto verifiedLayers = service.handleRequest(request(
        QStringLiteral("projection.layers.submit"), verifiedLayerSubmitParameters(service.sessionId()), token));
    require(verifiedLayers.contains(QStringLiteral("result")), "verified P3 target layer submit accepted");
    const auto verifiedFrame = service.handleRequest(request(QStringLiteral("projection.output.frame"),
                                                             {{QStringLiteral("output_id"), QStringLiteral("p4-window-projection")}}, token));
    const auto verifiedFrameResult = verifiedFrame.value(QStringLiteral("result")).toObject();
    require(verifiedFrameResult.value(QStringLiteral("p3_integration_status")).toString()
                == QStringLiteral("P3_SERVICE_PROJECTION_TARGET_VERIFIED"),
            "verified P3 target marker retained on output frame");
    require(verifiedFrameResult.value(QStringLiteral("spatial_target_id")).toString()
                == QStringLiteral("63f9b938-c56d-40d8-8880-9374e4ba35f2"),
            "verified P3 target identity retained on output frame");

    QJsonObject privateBatch = layerSubmitParameters(service.sessionId(), QStringLiteral("PRIVATE_SCREEN_ONLY"));
    const auto privateLayers = service.handleRequest(request(QStringLiteral("projection.layers.submit"), privateBatch, token));
    require(privateLayers.contains(QStringLiteral("error")), "private layer submit is rejected");
    require(!service.outputHasContent(), "private layer rejection safely clears output");

    const auto targetLoss = service.handleRequest(request(
        QStringLiteral("projection.output.clear"),
        {{QStringLiteral("session_id"), service.sessionId()}, {QStringLiteral("output_id"), QStringLiteral("p4-window-projection")}, {QStringLiteral("reason"), QStringLiteral("TARGET_LOST")}}, token));
    require(targetLoss.contains(QStringLiteral("result")), "target loss clear succeeds");
    require(!service.outputHasContent(), "target loss leaves no output content");

    require(service.handleRequest(request(QStringLiteral("projection.session.command"),
                                          commandParameters(service.sessionId(), QStringLiteral("INITIALIZE")), token))
                .contains(QStringLiteral("result")),
            "projection reinitializes after target loss");
    require(service.handleRequest(request(QStringLiteral("projection.session.command"),
                                          commandParameters(service.sessionId(), QStringLiteral("READY")), token))
                .contains(QStringLiteral("result")),
            "projection returns to ready after target loss");

    const auto cleared = service.handleRequest(request(
        QStringLiteral("projection.render"),
        renderParameters(service.sessionId(), QStringLiteral("NO_PROJECTION")), token));
    require(cleared.contains(QStringLiteral("error")), "no-projection render request is rejected");
    require(!service.outputHasContent(), "no-projection request leaves no output content");
    const auto staleFrame = service.handleRequest(request(QStringLiteral("projection.output.frame"),
                                                          {{QStringLiteral("output_id"), QStringLiteral("p4-window-projection")}}, token));
    require(staleFrame.contains(QStringLiteral("error")), "cleared output has no stale frame");
    require(staleFrame.value(QStringLiteral("error")).toObject().value(QStringLiteral("code")).toInt() == 4018,
            "cleared output uses the registered unavailable-frame code");

    const auto malformed = service.handleRequest(request(QStringLiteral("projection.render"),
                                                          {{QStringLiteral("fixture_id"), QStringLiteral("front-rectangle")}}, token));
    require(malformed.contains(QStringLiteral("error")), "malformed render request is rejected");
    require(malformed.value(QStringLiteral("error")).toObject().value(QStringLiteral("code")).toInt() == 9001,
            "malformed render request uses the registered validation code");

    QJsonObject unknownFixture = renderParameters(service.sessionId(), QStringLiteral("PUBLIC"));
    unknownFixture.insert(QStringLiteral("fixture_id"), QStringLiteral("not-a-fixture"));
    const auto invalidFixture = service.handleRequest(request(QStringLiteral("projection.render"), unknownFixture, token));
    require(invalidFixture.contains(QStringLiteral("error")), "unknown fixture is rejected");
    require(invalidFixture.value(QStringLiteral("error")).toObject().value(QStringLiteral("code")).toInt() == 9001,
            "unknown fixture uses the registered validation code");
}
