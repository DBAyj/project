#include "application/SpatialService.h"
#include "transport/SpatialJsonRpcServer.h"

#include <QCommandLineOption>
#include <QCommandLineParser>
#include <QCoreApplication>
#include <QDir>
#include <QTextStream>
#include <QTimer>

int main(int argc, char **argv)
{
    QCoreApplication application(argc, argv);
    QCoreApplication::setApplicationName(QStringLiteral("astra-spatial-service"));
    QCommandLineParser parser;
    parser.addHelpOption();
    const QCommandLineOption rootOption(QStringLiteral("root"), QStringLiteral("Project root"), QStringLiteral("path"), QDir::currentPath());
    const QCommandLineOption socketOption(QStringLiteral("socket"), QStringLiteral("Unix socket path"), QStringLiteral("path"));
    const QCommandLineOption tokenOption(QStringLiteral("token"), QStringLiteral("Capability token"), QStringLiteral("token"));
    const QCommandLineOption capabilitiesOption(QStringLiteral("capabilities"), QStringLiteral("Granted capabilities"), QStringLiteral("capabilities"));
    const QCommandLineOption fixtureOption(QStringLiteral("fixture"), QStringLiteral("Initial simulation fixture"), QStringLiteral("path"));
    parser.addOption(rootOption);
    parser.addOption(socketOption);
    parser.addOption(tokenOption);
    parser.addOption(capabilitiesOption);
    parser.addOption(fixtureOption);
    parser.process(application);

    const QString root = parser.value(rootOption);
    const QString socketPath = parser.isSet(socketOption) ? parser.value(socketOption) : root + QStringLiteral("/runtime/spatial/sockets/astra-spatial.sock");
    const QString token = parser.isSet(tokenOption) ? parser.value(tokenOption) : qEnvironmentVariable("ASTRA_SPATIAL_CAPABILITY_TOKEN");
    const QStringList capabilities = (parser.isSet(capabilitiesOption) ? parser.value(capabilitiesOption)
                                                                         : qEnvironmentVariable("ASTRA_SPATIAL_CAPABILITIES"))
                                         .split(',', Qt::SkipEmptyParts);
    const QString fixture = parser.isSet(fixtureOption) ? parser.value(fixtureOption) : root + QStringLiteral("/assets/spatial-fixtures/desk-front.png");
    if (token.isEmpty() || capabilities.isEmpty()) {
        QTextStream(stderr) << "Capability token and granted capabilities are required\n";
        return 2;
    }

    astra::spatial::SpatialService service(root, root + QStringLiteral("/runtime/audit/spatial-audit.jsonl"));
    if (!service.start(astra::spatial::FrameSourceType::Simulation, fixture).ok) {
        QTextStream(stderr) << "Unable to start simulation frame source\n";
        return 3;
    }
    astra::spatial::SpatialJsonRpcServer server(&service, token, capabilities);
    if (!server.listen(socketPath)) {
        QTextStream(stderr) << "Unable to listen on spatial socket\n";
        return 4;
    }
    QObject::connect(&application, &QCoreApplication::aboutToQuit, [&] {
        server.close();
        service.stop();
    });
    QTimer frameTimer;
    frameTimer.setInterval(1000 / 15);
    QObject::connect(&frameTimer, &QTimer::timeout, [&] {
        if (service.snapshot().state != astra::spatial::SpatialServiceState::Tracking) service.detect();
        server.publishStateChangedIfChanged();
    });
    frameTimer.start();
    QTextStream(stdout) << "astra-spatial-service socket=" << socketPath << '\n';
    return application.exec();
}
