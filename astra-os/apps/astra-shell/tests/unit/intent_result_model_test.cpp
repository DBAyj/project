#include "models/IntentResultModel.h"

#include <cassert>

int main()
{
    astra::shell::IntentResultModel model;
    astra::shell::IntentServiceResult result;
    result.transportOk = true;
    result.intent = QStringLiteral("project_3d_model");
    result.confidence = 0.97;
    result.executionPolicy = QStringLiteral("REQUIRE_CONFIRMATION");
    result.requiresConfirmation = true;
    result.ambiguous = false;
    result.candidates = {QVariantMap {{QStringLiteral("intent"), QStringLiteral("project_3d_model")}}};
    result.slotValues = {{QStringLiteral("target_space"), QStringLiteral("desk")}};
    result.ruleEngineUsed = true;
    result.localModelUsed = true;
    result.durationMs = 5.5;
    result.confirmationId = QStringLiteral("confirmation-id");
    result.confirmationMessage = QStringLiteral("是否继续？");

    model.apply(result);
    assert(model.serviceStatus() == QStringLiteral("ONLINE"));
    assert(model.intent() == QStringLiteral("project_3d_model"));
    assert(model.confidence() == 0.97);
    assert(model.requiresConfirmation());
    assert(model.slotValues().value("target_space") == QStringLiteral("desk"));
    assert(model.ruleEngineStatus() == QStringLiteral("READY"));
    assert(model.localModelStatus() == QStringLiteral("READY"));

    model.setOfflineFallback(QStringLiteral("连接失败"));
    assert(model.serviceStatus() == QStringLiteral("OFFLINE_FALLBACK"));
    assert(model.errorMessage() == QStringLiteral("连接失败"));
    assert(!model.requiresConfirmation());
}
