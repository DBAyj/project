#include "application/SpatialService.h"
#include "transport/SpatialJsonRpcServer.h"

#include <QCoreApplication>
#include <QElapsedTimer>
#include <QJsonArray>
#include <QJsonDocument>
#include <QLocalSocket>
#include <QTemporaryDir>
#include <QThread>
#include <QUuid>

#include <cassert>

namespace {

QJsonObject request(QLocalSocket &socket, QCoreApplication &app, const QString &method, const QJsonObject &params = {})
{
    const QJsonObject value {{QStringLiteral("jsonrpc"), QStringLiteral("2.0")},
                             {QStringLiteral("id"), method},
                             {QStringLiteral("trace_id"), QUuid::createUuid().toString(QUuid::WithoutBraces)},
                             {QStringLiteral("method"), method},
                             {QStringLiteral("params"), params},
                             {QStringLiteral("security_context"), QJsonObject {{QStringLiteral("capability_token"), QStringLiteral("test-token")}}}};
    assert(socket.write(QJsonDocument(value).toJson(QJsonDocument::Compact) + '\n') > 0);
    assert(socket.waitForBytesWritten(1000));
    QElapsedTimer timer;
    timer.start();
    while (timer.elapsed() < 1000) {
        while (socket.canReadLine()) {
            const QJsonObject response = QJsonDocument::fromJson(socket.readLine()).object();
            if (!response.contains(QStringLiteral("method"))) return response;
        }
        app.processEvents();
        QThread::msleep(5);
    }
    assert(false);
    return {};
}

QJsonObject notification(QLocalSocket &socket, QCoreApplication &app)
{
    QElapsedTimer timer;
    timer.start();
    while (timer.elapsed() < 1000) {
        while (socket.canReadLine()) {
            const QJsonObject value = QJsonDocument::fromJson(socket.readLine()).object();
            if (value.contains(QStringLiteral("method"))) return value;
        }
        app.processEvents();
        QThread::msleep(5);
    }
    assert(false);
    return {};
}

} // namespace

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);
    QTemporaryDir directory;
    const QString root = QStringLiteral(ASTRA_SOURCE_ROOT);
    const QString socketPath = directory.filePath("astra-spatial.sock");
    astra::spatial::SpatialService service(root, directory.filePath("audit/events.jsonl"));
    astra::spatial::SpatialJsonRpcServer server(&service, QStringLiteral("test-token"),
                                                {QStringLiteral("spatial.read"), QStringLiteral("spatial.control"), QStringLiteral("spatial.observer.write")});
    assert(server.listen(socketPath));

    QLocalSocket client;
    client.connectToServer(socketPath);
    assert(client.waitForConnected(1000));
    app.processEvents();
    const auto health = request(client, app, QStringLiteral("spatial.health"));
    assert(health.value("result").toObject().value("status") == QStringLiteral("NOT_READY"));
    const auto subscribed = request(client, app, QStringLiteral("event.subscribe"),
                                    {{QStringLiteral("event"), QStringLiteral("spatial.state_changed")}});
    assert(subscribed.value(QStringLiteral("result")).toObject().value(QStringLiteral("event")) == QStringLiteral("spatial.state_changed"));

    const auto started = request(client, app, QStringLiteral("spatial.source.start"),
                                 {{QStringLiteral("source"), QStringLiteral("SIMULATION")},
                                  {QStringLiteral("location"), root + QStringLiteral("/assets/spatial-fixtures/desk-front.png")}});
    assert(started.contains(QStringLiteral("result")));
    const auto stateChanged = notification(client, app);
    assert(stateChanged.value(QStringLiteral("method")) == QStringLiteral("spatial.state_changed"));
    assert(stateChanged.value(QStringLiteral("params")).toObject().value(QStringLiteral("state")) == QStringLiteral("READY"));
    const auto detected = request(client, app, QStringLiteral("spatial.detect"));
    assert(detected.value("result").toObject().value("surface_candidates").toInt() > 0);
    assert(request(client, app, QStringLiteral("spatial.select")).contains(QStringLiteral("result")));
    const QJsonArray corners = request(client, app, QStringLiteral("spatial.state")).value("result").toObject().value("target").toObject().value("corners").toArray();
    assert(request(client, app, QStringLiteral("spatial.calibrate"), {{QStringLiteral("corners"), corners}}).value("result").toObject().value("state") == QStringLiteral("TRACKING"));

    const auto denied = QJsonObject {{QStringLiteral("jsonrpc"), QStringLiteral("2.0")},
                                      {QStringLiteral("id"), QStringLiteral("denied")},
                                      {QStringLiteral("trace_id"), QUuid::createUuid().toString(QUuid::WithoutBraces)},
                                      {QStringLiteral("method"), QStringLiteral("spatial.state")},
                                      {QStringLiteral("params"), QJsonObject {}},
                                      {QStringLiteral("security_context"), QJsonObject {{QStringLiteral("capability_token"), QStringLiteral("wrong")}}}};
    assert(client.write(QJsonDocument(denied).toJson(QJsonDocument::Compact) + '\n') > 0);
    assert(client.waitForBytesWritten(1000));
    QElapsedTimer timer;
    timer.start();
    QJsonObject deniedResponse;
    while (timer.elapsed() < 1000 && deniedResponse.isEmpty()) {
        while (client.canReadLine()) {
            const QJsonObject response = QJsonDocument::fromJson(client.readLine()).object();
            if (response.contains(QStringLiteral("error"))) {
                deniedResponse = response;
                break;
            }
        }
        app.processEvents();
        QThread::msleep(5);
    }
    assert(deniedResponse.value("error").toObject().value("code").toInt() == 5001);

    const auto invalid = request(client, app, QStringLiteral("spatial.calibrate"), QJsonObject {});
    assert(invalid.value("error").toObject().value("code").toInt() == 3504);
    client.disconnectFromServer();
    client.waitForDisconnected(1000);
    app.processEvents();
    server.close();

    astra::spatial::SpatialJsonRpcServer readOnlyServer(&service, QStringLiteral("test-token"), {QStringLiteral("spatial.read")});
    assert(readOnlyServer.listen(socketPath));
    QLocalSocket readOnlyClient;
    readOnlyClient.connectToServer(socketPath);
    assert(readOnlyClient.waitForConnected(1000));
    const auto deniedControl = request(readOnlyClient, app, QStringLiteral("spatial.detect"));
    assert(deniedControl.value(QStringLiteral("error")).toObject().value(QStringLiteral("code")).toInt() == 5001);
    readOnlyClient.disconnectFromServer();
    readOnlyClient.waitForDisconnected(1000);
    readOnlyServer.close();
}
