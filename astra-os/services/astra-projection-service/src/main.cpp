#include "astra/projection/service/ProjectionService.h"

#include <QCommandLineParser>
#include <QFile>
#include <QGuiApplication>

namespace {

QString readToken(const QString &path)
{
    if (path.isEmpty()) return QString::fromLatin1(astra::projection::service::kFixtureCapabilityToken);
    QFile file {path};
    if (!file.open(QIODevice::ReadOnly)) return {};
    return QString::fromUtf8(file.readAll()).trimmed();
}

} // namespace

int main(int argc, char *argv[])
{
    QGuiApplication application(argc, argv);
    QCommandLineParser parser;
    parser.addOption({QStringLiteral("socket"), QStringLiteral("Local Unix socket name"), QStringLiteral("name"), QStringLiteral("astra-p4-projection")});
    parser.addOption({QStringLiteral("capability-token-file"), QStringLiteral("Projection capability token file"), QStringLiteral("path")});
    parser.process(application);

    const QString capabilityToken = readToken(parser.value(QStringLiteral("capability-token-file")));
    if (capabilityToken.size() < 32) {
        qCritical().noquote() << "projection_service_credentials_invalid=true";
        return 1;
    }
    astra::projection::service::ProjectionService service {capabilityToken};
    QString error;
    if (!service.listen(parser.value(QStringLiteral("socket")), &error)) {
        qCritical().noquote() << "projection_service_listen_failed=" + error;
        return 1;
    }
    qInfo().noquote() << "projection_service_ready=true p3_integration_status=P3_REAL_SPATIAL_INTEGRATION_NOT_VERIFIED";
    return application.exec();
}
