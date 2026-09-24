#include "astra/spatial_ui/service/SpatialUIService.h"
#include "astra/spatial_ui/service/SpatialUIConfiguration.h"

#include "astra/common/PlatformPaths.h"

#include <QCommandLineParser>
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QHostAddress>

#include <memory>

namespace {

QString readToken(const QString &path)
{
    QFile file {path};
    if (path.isEmpty() || !file.open(QIODevice::ReadOnly)) return {};
    return QString::fromUtf8(file.readAll()).trimmed();
}

} // namespace

int main(int argc, char **argv)
{
    QCoreApplication application {argc, argv};
    QCoreApplication::setApplicationName(QStringLiteral("astra-spatial-ui-service"));
    QCommandLineParser parser;
    parser.addHelpOption();
    parser.addOption({QStringLiteral("project-root"), QStringLiteral("Explicit development project root"), QStringLiteral("path")});
    parser.addOption({QStringLiteral("config-dir"), QStringLiteral("Spatial UI configuration directory"), QStringLiteral("path")});
    parser.addOption({QStringLiteral("schema-dir"), QStringLiteral("Spatial UI configuration Schema directory"), QStringLiteral("path")});
    parser.addOption({QStringLiteral("socket"), QStringLiteral("Unix socket path"), QStringLiteral("path")});
    parser.addOption({QStringLiteral("state"), QStringLiteral("UI state path"), QStringLiteral("path")});
    parser.addOption({QStringLiteral("audit"), QStringLiteral("Audit log path"), QStringLiteral("path")});
    parser.addOption({QStringLiteral("client-token-file"), QStringLiteral("Spatial UI client capability token file"), QStringLiteral("path")});
    parser.addOption({QStringLiteral("supervisor-token-file"), QStringLiteral("Supervisor shutdown token file"), QStringLiteral("path")});
    parser.addOption({QStringLiteral("projection-socket"), QStringLiteral("P4 projection Unix socket path"), QStringLiteral("path")});
    parser.addOption({QStringLiteral("projection-token-file"), QStringLiteral("P4 projection capability token file"), QStringLiteral("path")});
    parser.addOption({QStringLiteral("spatial-socket"), QStringLiteral("P3 spatial Unix socket path"), QStringLiteral("path")});
    parser.addOption({QStringLiteral("spatial-token-file"), QStringLiteral("P3 spatial read capability token file"), QStringLiteral("path")});
    parser.addOption({QStringLiteral("http-address"), QStringLiteral("Explicit development HTTP bind address"), QStringLiteral("address")});
    parser.addOption({QStringLiteral("http-port"), QStringLiteral("Development HTTP port"), QStringLiteral("port"), QStringLiteral("0")});
    parser.process(application);

    const QString projectRoot = parser.isSet(QStringLiteral("project-root"))
        ? parser.value(QStringLiteral("project-root")) : qEnvironmentVariable("ASTRA_PROJECT_ROOT");
    const auto paths = astra::common::PlatformPaths::forCurrentHost(projectRoot);
    const QString socketPath = parser.isSet(QStringLiteral("socket")) ? parser.value(QStringLiteral("socket")) : paths.socketPath;
    const QString statePath = parser.isSet(QStringLiteral("state")) ? parser.value(QStringLiteral("state")) : paths.statePath;
    const QString auditPath = parser.isSet(QStringLiteral("audit")) ? parser.value(QStringLiteral("audit")) : paths.auditPath;
    const QString configurationDirectory = parser.isSet(QStringLiteral("config-dir"))
        ? parser.value(QStringLiteral("config-dir")) : paths.configurationDirectory;
    const QString schemaDirectory = parser.isSet(QStringLiteral("schema-dir"))
        ? parser.value(QStringLiteral("schema-dir")) : paths.schemaDirectory;
    const QString clientToken = readToken(parser.value(QStringLiteral("client-token-file")));
    const QString supervisorToken = readToken(parser.value(QStringLiteral("supervisor-token-file")));
    const QString projectionToken = readToken(parser.value(QStringLiteral("projection-token-file")));
    const QString spatialToken = readToken(parser.value(QStringLiteral("spatial-token-file")));
    if (clientToken.size() < 32 || supervisorToken.size() < 32 || clientToken == supervisorToken) {
        qCritical().noquote() << "spatial_ui_credentials_invalid=true";
        return 1;
    }
    std::shared_ptr<astra::spatial_ui::service::ProjectionGateway> projectionGateway;
    if (!parser.value(QStringLiteral("projection-socket")).isEmpty() && !projectionToken.isEmpty()) {
        projectionGateway = std::make_shared<astra::spatial_ui::service::UnixProjectionGateway>(
            parser.value(QStringLiteral("projection-socket")), projectionToken,
            parser.value(QStringLiteral("spatial-socket")), spatialToken);
    }
    auto configuration = astra::spatial_ui::service::SpatialUIConfiguration::load(configurationDirectory, schemaDirectory);
    configuration.options.policyBindingKey = projectionToken;
    if (!projectionGateway) {
        configuration.options.projectionTargetEnabled = false;
        configuration.options.configurationStatus = QStringLiteral("SAFE_DEFAULTS");
        configuration.options.configurationWarning = QStringLiteral("P4 projection gateway is unavailable; projection is disabled.");
    }
    if (!configuration.options.configurationWarning.isEmpty()) {
        qWarning().noquote() << "spatial_ui_configuration_warning=" + configuration.options.configurationWarning;
    }
    astra::spatial_ui::service::SpatialUIService service {statePath, auditPath, clientToken, supervisorToken,
                                                          std::move(projectionGateway), configuration.options};
    service.setShutdownHandler([&application] { application.quit(); });
    QString error;
    if (!service.listen(socketPath, &error)) return 1;
    bool portOk = false;
    const auto port = parser.value(QStringLiteral("http-port")).toUShort(&portOk);
    const QString httpAddressValue = parser.value(QStringLiteral("http-address"));
    const QHostAddress httpAddress {httpAddressValue};
    if (!portOk || (!httpAddressValue.isEmpty() && (httpAddress.isNull() || !service.listenHttp(httpAddress, port, &error)))) return 1;
    qInfo().noquote() << "spatial_ui_socket=" + service.socketPath()
                      << (httpAddressValue.isEmpty() ? QStringLiteral("development_http=disabled")
                                                     : "development_http=http://" + httpAddress.toString() + ":" + QString::number(service.httpPort()))
                      << "configuration_status=" + configuration.options.configurationStatus
                      << "p4_release_status=P4_RELEASE_BASELINE_FINAL";
    QObject::connect(&application, &QCoreApplication::aboutToQuit, &application, [&service] { service.close(); });
    return application.exec();
}
