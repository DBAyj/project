#include "clients/IntentServiceClient.h"

#include <QCoreApplication>
#include <QDir>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLocalServer>
#include <QLocalSocket>
#include <QTemporaryDir>

#include <cassert>
#include <future>

namespace {

QJsonObject resultEnvelope(const QString &rpcId)
{
    return {{"jsonrpc", "2.0"},
            {"id", rpcId},
            {"result",
             QJsonObject {{"schema_version", "2.0"},
                          {"request_id", "7abf7b1e-dc4f-4b7c-96cc-c3dd628e1ca9"},
                          {"trace_id", "bf489ffc-6bd5-464d-a1b7-5655863cb8c2"},
                          {"intent", "project_3d_model"},
                          {"confidence", 0.97},
                          {"execution_policy", "AUTO_EXECUTE"},
                          {"requires_confirmation", false},
                          {"ambiguous", false},
                          {"candidates", QJsonArray {QJsonObject {{"intent", "project_3d_model"}, {"confidence", 0.97}, {"source", "RULE_ENGINE"}}}},
                          {"slots", QJsonObject {{"target_space", "desk"}, {"model_id", "demo-device"}, {"privacy_level", "PUBLIC"}}},
                          {"normalized_text", "把设备模型投到桌面上"},
                          {"processing", QJsonObject {{"rule_engine_used", true}, {"local_model_used", true}, {"fallback_used", false}, {"cache_hit", false}, {"duration_ms", 4.5}}},
                          {"warnings", QJsonArray {}},
                          {"error", QJsonValue::Null},
                          {"clarification", QJsonValue::Null},
                          {"confirmation", QJsonValue::Null},
                          {"created_at", "2026-07-14T13:40:00Z"}}}};
}

} // namespace

int main(int argc, char **argv)
{
    QCoreApplication application(argc, argv);
    QTemporaryDir directory;
    assert(directory.isValid());
    const QString socketPath = QDir::temp().filePath(QStringLiteral("aic-%1.sock").arg(QCoreApplication::applicationPid()));
    QLocalServer::removeServer(socketPath);
    QLocalServer server;
    assert(server.listen(socketPath));

    astra::shell::IntentServiceClient client(socketPath, QStringLiteral("test-capability"), 1000);
    auto pending = std::async(std::launch::async, [&client] {
        return client.analyze(QStringLiteral("把设备模型投到桌面上"),
                              QStringLiteral("PUBLIC"),
                              QStringLiteral("IDLE"),
                              QStringLiteral("development_workspace"),
                              QStringLiteral("demo-device"));
    });

    assert(server.waitForNewConnection(1000));
    QLocalSocket *connection = server.nextPendingConnection();
    assert(connection != nullptr);
    assert(connection->waitForReadyRead(1000));
    const QJsonObject request = QJsonDocument::fromJson(connection->readLine()).object();
    assert(request.value("jsonrpc") == QStringLiteral("2.0"));
    assert(request.value("method") == QStringLiteral("intent.parse"));
    assert(request.value("security_context").toObject().value("capability_token") == QStringLiteral("test-capability"));
    const QJsonObject params = request.value("params").toObject();
    assert(params.value("raw_text") == QStringLiteral("把设备模型投到桌面上"));
    assert(params.value("current_context").toObject().value("current_privacy_level") == QStringLiteral("PUBLIC"));

    const QByteArray response = QJsonDocument(resultEnvelope(request.value("id").toString())).toJson(QJsonDocument::Compact) + '\n';
    assert(connection->write(response) == response.size());
    assert(connection->waitForBytesWritten(1000));
    connection->disconnectFromServer();
    const astra::shell::IntentServiceResult result = pending.get();
    assert(result.transportOk);
    assert(result.intent == QStringLiteral("project_3d_model"));
    assert(result.executionPolicy == QStringLiteral("AUTO_EXECUTE"));
    assert(result.confidence == 0.97);
    assert(result.slotValues.value("target_space") == QStringLiteral("desk"));
    assert(result.ruleEngineUsed);
    assert(result.localModelUsed);

    astra::shell::IntentServiceClient unavailable(QDir::temp().filePath(QStringLiteral("aic-missing.sock")), QStringLiteral("test-capability"), 50);
    const auto unavailableResult = unavailable.analyze(QStringLiteral("查看系统状态"),
                                                       QStringLiteral("PUBLIC"),
                                                       QStringLiteral("IDLE"),
                                                       QStringLiteral("development_workspace"),
                                                       QStringLiteral("demo-device"));
    assert(!unavailableResult.transportOk);
    assert(unavailableResult.errorCode == 2003);
}
