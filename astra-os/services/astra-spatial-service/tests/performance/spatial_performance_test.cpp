#include "input/FrameSource.h"
#include "application/SpatialService.h"
#include "transport/SpatialJsonRpcServer.h"
#include "vision/SurfaceDetector.h"

#include <QCoreApplication>
#include <QDir>
#include <QElapsedTimer>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLocalSocket>
#include <QTemporaryDir>
#include <QThread>
#include <QUuid>

#include <algorithm>
#include <cassert>
#include <cmath>
#include <vector>

namespace {

double percentile(std::vector<double> values)
{
    std::sort(values.begin(), values.end());
    return values[static_cast<std::size_t>(std::ceil(values.size() * 0.95) - 1.0)];
}

QJsonObject rpcRequest(QLocalSocket &socket,
                       QCoreApplication &application,
                       const QString &method,
                       const QJsonObject &params = {})
{
    const QJsonObject request {{QStringLiteral("jsonrpc"), QStringLiteral("2.0")},
                               {QStringLiteral("id"), QUuid::createUuid().toString(QUuid::WithoutBraces)},
                               {QStringLiteral("trace_id"), QUuid::createUuid().toString(QUuid::WithoutBraces)},
                               {QStringLiteral("method"), method},
                               {QStringLiteral("params"), params},
                               {QStringLiteral("security_context"), QJsonObject {{QStringLiteral("capability_token"), QStringLiteral("performance-token")}}}};
    const QByteArray payload = QJsonDocument(request).toJson(QJsonDocument::Compact) + '\n';
    assert(socket.write(payload) == payload.size());
    assert(socket.waitForBytesWritten(1000));
    QElapsedTimer timeout;
    timeout.start();
    while (timeout.elapsed() < 1000) {
        application.processEvents();
        if (!socket.canReadLine()) socket.waitForReadyRead(10);
        if (socket.canReadLine()) {
            const QJsonObject response = QJsonDocument::fromJson(socket.readLine()).object();
            if (response.contains(QStringLiteral("result")) || response.contains(QStringLiteral("error"))) return response;
        }
        QThread::msleep(1);
    }
    assert(false);
    return {};
}

QJsonObject stateNotification(QLocalSocket &socket, QCoreApplication &application)
{
    QElapsedTimer timeout;
    timeout.start();
    while (timeout.elapsed() < 1000) {
        application.processEvents();
        if (!socket.canReadLine()) socket.waitForReadyRead(10);
        while (socket.canReadLine()) {
            const QJsonObject notification = QJsonDocument::fromJson(socket.readLine()).object();
            if (notification.value(QStringLiteral("method")) == QStringLiteral("spatial.state_changed")) return notification;
        }
        QThread::msleep(1);
    }
    assert(false);
    return {};
}

} // namespace

int main(int argc, char **argv)
{
    QCoreApplication application(argc, argv);
    const QString root = QStringLiteral(ASTRA_SOURCE_ROOT);
    astra::spatial::FrameSourceController source(root);
    astra::spatial::SurfaceDetector detector;
    assert(source.start(astra::spatial::FrameSourceType::Simulation,
                        root + QStringLiteral("/assets/spatial-fixtures/desk-front.png")).ok);
    const auto frame = source.nextFrame();
    assert(frame.ok);
    std::vector<double> preprocessMs;
    std::vector<double> detectMs;
    QElapsedTimer total;
    total.start();
    for (int iteration = 0; iteration < 40; ++iteration) {
        QElapsedTimer timer;
        timer.start();
        assert(detector.preprocess(frame.value).ok);
        preprocessMs.push_back(static_cast<double>(timer.nsecsElapsed()) / 1'000'000.0);
        timer.restart();
        assert(detector.detect(frame.value).ok);
        detectMs.push_back(static_cast<double>(timer.nsecsElapsed()) / 1'000'000.0);
    }
    const double preprocessingP95 = percentile(preprocessMs);
    const double detectionP95 = percentile(detectMs);
    const double pipelineP95 = preprocessingP95 + detectionP95;
    const double fps = 40'000.0 / static_cast<double>(total.elapsed());
    assert(preprocessingP95 <= 20.0);
    assert(detectionP95 <= 40.0);
    assert(pipelineP95 <= 70.0);
    assert(fps >= 15.0);
    QTemporaryDir eventDirectory;
    astra::spatial::SpatialService eventService(root, eventDirectory.filePath("audit/events.jsonl"));
    const QString socketPath = QDir::temp().filePath(QStringLiteral("ap3-") + QUuid::createUuid().toString(QUuid::Id128) + QStringLiteral(".sock"));
    astra::spatial::SpatialJsonRpcServer server(&eventService,
                                                QStringLiteral("performance-token"),
                                                {QStringLiteral("spatial.read"), QStringLiteral("spatial.control")});
    assert(server.listen(socketPath));
    QLocalSocket subscriber;
    subscriber.connectToServer(socketPath);
    assert(subscriber.waitForConnected(1000));
    const auto subscribed = rpcRequest(subscriber, application, QStringLiteral("event.subscribe"),
                                       {{QStringLiteral("event"), QStringLiteral("spatial.state_changed")}});
    assert(subscribed.contains(QStringLiteral("result")));
    QLocalSocket controller;
    controller.connectToServer(socketPath);
    assert(controller.waitForConnected(1000));
    assert(rpcRequest(controller, application, QStringLiteral("spatial.source.start"),
                      {{QStringLiteral("source"), QStringLiteral("SIMULATION")},
                       {QStringLiteral("location"), root + QStringLiteral("/assets/spatial-fixtures/desk-front.png")}})
               .contains(QStringLiteral("result")));
    const auto started = stateNotification(subscriber, application);
    assert(started.value(QStringLiteral("params")).toObject().value(QStringLiteral("state")) == QStringLiteral("READY"));
    std::vector<double> eventLatencyMs;
    for (int iteration = 0; iteration < 40; ++iteration) {
        if (iteration > 0) {
            assert(rpcRequest(controller, application, QStringLiteral("spatial.reset")).contains(QStringLiteral("result")));
            const auto ready = stateNotification(subscriber, application);
            assert(ready.value(QStringLiteral("params")).toObject().value(QStringLiteral("state")) == QStringLiteral("READY"));
        }
        QElapsedTimer timer;
        timer.start();
        assert(rpcRequest(controller, application, QStringLiteral("spatial.detect")).contains(QStringLiteral("result")));
        const auto detected = stateNotification(subscriber, application);
        assert(detected.value(QStringLiteral("params")).toObject().value(QStringLiteral("state")) == QStringLiteral("DETECTING"));
        eventLatencyMs.push_back(static_cast<double>(timer.nsecsElapsed()) / 1'000'000.0);
    }
    controller.disconnectFromServer();
    subscriber.disconnectFromServer();
    application.processEvents();
    server.close();
    assert(eventService.stop().ok);
    const double eventLatencyP95 = percentile(eventLatencyMs);
    assert(eventLatencyP95 <= 100.0);
    const QString output = qEnvironmentVariable("ASTRA_P3_PERFORMANCE_PATH", root + QStringLiteral("/runtime/tmp/p3-performance.json"));
    QDir().mkpath(QFileInfo(output).dir().path());
    QFile file(output);
    assert(file.open(QIODevice::WriteOnly | QIODevice::Truncate));
    const QJsonObject evidence {{QStringLiteral("status"), QStringLiteral("PASSED")},
                                {QStringLiteral("preprocessing_p95_ms"), preprocessingP95},
                                {QStringLiteral("detection_p95_ms"), detectionP95},
                                {QStringLiteral("pipeline_p95_ms"), pipelineP95},
                                {QStringLiteral("fps"), fps},
                                {QStringLiteral("event_latency_p95_ms"), eventLatencyP95}};
    assert(file.write(QJsonDocument(evidence).toJson(QJsonDocument::Compact) + '\n') > 0);
}
