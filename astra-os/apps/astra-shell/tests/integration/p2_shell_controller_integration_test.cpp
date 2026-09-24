#include "clients/IntentServiceClient.h"
#include "controllers/ShellController.h"

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
#include <thread>
#include <vector>

namespace {

QJsonObject intentResult(const QString &intent,
                         const QString &policy,
                         const QJsonObject &confirmation = {})
{
    return {{"request_id", "7abf7b1e-dc4f-4b7c-96cc-c3dd628e1ca9"},
            {"trace_id", "bf489ffc-6bd5-464d-a1b7-5655863cb8c2"},
            {"intent", intent},
            {"confidence", 0.97},
            {"execution_policy", policy},
            {"requires_confirmation", policy == QStringLiteral("REQUIRE_CONFIRMATION")},
            {"ambiguous", false},
            {"candidates", QJsonArray {}},
            {"slots", QJsonObject {{"target_space", "desk"}, {"model_id", "demo-device"}, {"privacy_level", "PUBLIC"}}},
            {"normalized_text", intent},
            {"processing", QJsonObject {{"rule_engine_used", true}, {"local_model_used", true}, {"fallback_used", false}, {"cache_hit", false}, {"duration_ms", 2.0}}},
            {"warnings", QJsonArray {}},
            {"error", QJsonValue::Null},
            {"clarification", QJsonValue::Null},
            {"confirmation", confirmation.isEmpty() ? QJsonValue::Null : QJsonValue(confirmation)}};
}

class ScriptedServer {
public:
    ScriptedServer(QString path, std::vector<QJsonObject> results)
        : path_(std::move(path)), results_(std::move(results)), thread_([this] { run(); })
    {
        ready_.get_future().wait();
    }

    ~ScriptedServer()
    {
        if (thread_.joinable()) thread_.join();
        QLocalServer::removeServer(path_);
    }

private:
    void run()
    {
        QLocalServer::removeServer(path_);
        QLocalServer server;
        assert(server.listen(path_));
        ready_.set_value();
        for (const QJsonObject &result : results_) {
            assert(server.waitForNewConnection(2000));
            QLocalSocket *connection = server.nextPendingConnection();
            assert(connection != nullptr);
            assert(connection->waitForReadyRead(1000));
            const QJsonObject request = QJsonDocument::fromJson(connection->readLine()).object();
            const QJsonObject response {{"jsonrpc", "2.0"}, {"id", request.value("id")}, {"result", result}};
            const QByteArray payload = QJsonDocument(response).toJson(QJsonDocument::Compact) + '\n';
            assert(connection->write(payload) == payload.size());
            assert(connection->waitForBytesWritten(1000));
            connection->disconnectFromServer();
            connection->deleteLater();
        }
    }

    QString path_;
    std::vector<QJsonObject> results_;
    std::promise<void> ready_;
    std::thread thread_;
};

} // namespace

int main(int argc, char **argv)
{
    QCoreApplication application(argc, argv);
    QTemporaryDir directory;
    assert(directory.isValid());
    astra::shell::SimulatorConfig config;
    const QString socketPath = QDir::temp().filePath(QStringLiteral("aic-controller-%1.sock").arg(QCoreApplication::applicationPid()));
    const QJsonObject confirmation {{"confirmation_id", "bf489ffc-6bd5-464d-a1b7-5655863cb8c2"},
                                    {"message", "停止展示？"},
                                    {"expires_at", "2026-07-14T13:40:30Z"}};
    {
        ScriptedServer server(socketPath,
                              {intentResult(QStringLiteral("project_3d_model"), QStringLiteral("AUTO_EXECUTE")),
                               intentResult(QStringLiteral("stop_projection"), QStringLiteral("REQUIRE_CONFIRMATION"), confirmation),
                               QJsonObject {{"status", "ACCEPTED"}}});
        astra::shell::ShellController controller(config,
                                                 directory.filePath("audit/p2.jsonl"),
                                                 astra::shell::IntentServiceClient(socketPath, QStringLiteral("test-capability"), 1000),
                                                 true);
        controller.submit(QStringLiteral("把设备模型投到桌面上"), QStringLiteral("PUBLIC"));
        assert(controller.projectionState() == QStringLiteral("ACTIVE"));
        assert(controller.intentResult()->serviceStatus() == QStringLiteral("ONLINE"));

        controller.submit(QStringLiteral("停止投影"), QStringLiteral("PUBLIC"));
        assert(controller.projectionState() == QStringLiteral("ACTIVE"));
        assert(controller.currentTaskStatus() == QStringLiteral("AWAITING_CONFIRMATION"));
        controller.confirmIntent();
        assert(controller.projectionState() == QStringLiteral("IDLE"));
    }

    {
        // The scripted result extracts PUBLIC from the text; it must not loosen the selected level.
        ScriptedServer server(socketPath, {intentResult(QStringLiteral("project_3d_model"), QStringLiteral("AUTO_EXECUTE"))});
        astra::shell::ShellController controller(config,
                                                 directory.filePath("audit/p2-privacy.jsonl"),
                                                 astra::shell::IntentServiceClient(socketPath, QStringLiteral("test-capability"), 1000),
                                                 true);
        controller.submit(QStringLiteral("把设备模型投到桌面上，公开展示"), QStringLiteral("PRIVATE_SCREEN_ONLY"));
        assert(controller.currentPrivacyLevel() == QStringLiteral("PRIVATE_SCREEN_ONLY"));
        assert(controller.projectionState() == QStringLiteral("IDLE"));
        assert(controller.lastErrorCode() == 4301);
    }

    astra::shell::ShellController offline(config,
                                          directory.filePath("audit/offline.jsonl"),
                                          astra::shell::IntentServiceClient(QDir::temp().filePath("missing-p2.sock"),
                                                                           QStringLiteral("test-capability"),
                                                                           20),
                                          true);
    offline.submit(QStringLiteral("开始投影"), QStringLiteral("PUBLIC"));
    assert(offline.projectionState() == QStringLiteral("IDLE"));
    assert(offline.lastErrorCode() == 2004);
    offline.submit(QStringLiteral("查看系统状态"), QStringLiteral("PUBLIC"));
    assert(offline.currentIntent() == QStringLiteral("show_system_status"));
    assert(offline.currentTaskStatus() == QStringLiteral("COMPLETED"));
}
