#include "services/ConfigurationService.h"

#include "astra/common/ErrorCode.h"

#include <QDir>
#include <QFile>
#include <QHash>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QRegularExpression>
#include <QSet>
#include <QStringList>

namespace astra::shell {
namespace {

ConfigurationLoadResult invalidConfiguration(const QString &warning)
{
    ConfigurationLoadResult result;
    result.warning = warning;
    result.errorCode = static_cast<int>(astra::common::ErrorCode::ConfigurationInvalid);
    result.usingFallbackConfiguration = true;
    return result;
}

QString unquote(QString value)
{
    value = value.trimmed();
    if (value.size() >= 2 && ((value.startsWith('"') && value.endsWith('"'))
                              || (value.startsWith('\'') && value.endsWith('\'')))) {
        value = value.mid(1, value.size() - 2);
    }
    return value;
}

bool parseYamlScalars(const QString &source, QHash<QString, QString> &values)
{
    const QSet<QString> containers {
        QStringLiteral("application"), QStringLiteral("display"), QStringLiteral("display.phone"),
        QStringLiteral("display.projection"), QStringLiteral("projection"), QStringLiteral("authorization"),
        QStringLiteral("logging"), QStringLiteral("runtime"),
    };
    const QSet<QString> fields {
        QStringLiteral("application.name"), QStringLiteral("application.version"), QStringLiteral("application.locale"),
        QStringLiteral("display.phone.width"), QStringLiteral("display.phone.height"),
        QStringLiteral("display.projection.width"), QStringLiteral("display.projection.height"),
        QStringLiteral("display.projection.start_fullscreen"), QStringLiteral("projection.default_target"),
        QStringLiteral("projection.auto_rotate"), QStringLiteral("projection.rotation_speed"),
        QStringLiteral("projection.default_zoom"), QStringLiteral("authorization.simulated_authorized"),
        QStringLiteral("logging.level"), QStringLiteral("logging.audit_path"), QStringLiteral("runtime.recent_task_limit"),
    };
    const QRegularExpression entryPattern(QStringLiteral("^([A-Za-z][A-Za-z0-9_]*)\\s*:(.*)$"));
    QStringList hierarchy;

    for (const QString &rawLine : source.split('\n')) {
        if (rawLine.trimmed().isEmpty() || rawLine.trimmed().startsWith('#')) continue;
        if (rawLine.contains('\t')) return false;
        const int indentation = rawLine.size() - rawLine.trimmed().size();
        if (indentation % 2 != 0) return false;
        const int level = indentation / 2;
        if (level > hierarchy.size()) return false;
        while (hierarchy.size() > level) hierarchy.removeLast();

        const auto match = entryPattern.match(rawLine.trimmed());
        if (!match.hasMatch()) return false;
        const QString key = match.captured(1);
        const QString rawValue = match.captured(2).trimmed();
        const QString path = hierarchy.isEmpty() ? key : hierarchy.join('.') + QLatin1Char('.') + key;
        if (rawValue.isEmpty()) {
            if (!containers.contains(path)) return false;
            hierarchy.append(key);
            continue;
        }
        if (!fields.contains(path) || values.contains(path)) return false;
        values.insert(path, unquote(rawValue));
    }
    QSet<QString> configuredFields;
    for (auto iterator = values.cbegin(); iterator != values.cend(); ++iterator) configuredFields.insert(iterator.key());
    return configuredFields == fields;
}

bool validateSchemaDocument(const QString &schemaPath)
{
    if (schemaPath.isEmpty()) return true;
    QFile schemaFile(schemaPath);
    if (!schemaFile.open(QIODevice::ReadOnly)) return false;
    QJsonParseError error;
    const QJsonObject schema = QJsonDocument::fromJson(schemaFile.readAll(), &error).object();
    const QJsonObject properties = schema.value(QStringLiteral("properties")).toObject();
    const QJsonArray required = schema.value(QStringLiteral("required")).toArray();
    return error.error == QJsonParseError::NoError && schema.value(QStringLiteral("$schema")).toString() == QStringLiteral("https://json-schema.org/draft/2020-12/schema")
        && properties.contains(QStringLiteral("application")) && properties.contains(QStringLiteral("display"))
        && properties.contains(QStringLiteral("projection")) && properties.contains(QStringLiteral("authorization"))
        && properties.contains(QStringLiteral("logging")) && properties.contains(QStringLiteral("runtime")) && required.size() == 6;
}

bool parsePositiveInteger(const QString &value, int &parsed)
{
    static const QRegularExpression pattern(QStringLiteral("^[1-9][0-9]*$"));
    if (!pattern.match(value).hasMatch()) return false;
    bool valid = false;
    parsed = value.toInt(&valid);
    return valid;
}

bool parsePositiveDouble(const QString &value, double &parsed)
{
    bool valid = false;
    parsed = value.toDouble(&valid);
    return valid && parsed > 0.0;
}

bool parseBoolean(const QString &value, bool &parsed)
{
    if (value == QStringLiteral("true")) {
        parsed = true;
        return true;
    }
    if (value == QStringLiteral("false")) {
        parsed = false;
        return true;
    }
    return false;
}

} // namespace

ConfigurationLoadResult ConfigurationService::load(const QString &path, const QString &schemaPath)
{
    if (!validateSchemaDocument(schemaPath)) {
        return invalidConfiguration(QStringLiteral("Configuration schema unavailable or invalid; using safe defaults."));
    }
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return invalidConfiguration(QStringLiteral("Configuration unavailable; using safe defaults."));
    }

    QHash<QString, QString> values;
    if (!parseYamlScalars(QString::fromUtf8(file.readAll()), values)) {
        return invalidConfiguration(QStringLiteral("Configuration invalid; using safe defaults."));
    }

    SimulatorConfig config;
    config.applicationName = values.value(QStringLiteral("application.name"));
    config.applicationVersion = values.value(QStringLiteral("application.version"));
    config.locale = values.value(QStringLiteral("application.locale"));
    config.defaultTarget = values.value(QStringLiteral("projection.default_target"));
    config.loggingLevel = values.value(QStringLiteral("logging.level"));
    config.auditPath = values.value(QStringLiteral("logging.audit_path"));
    const QRegularExpression versionPattern(QStringLiteral("^[0-9]+\\.[0-9]+\\.[0-9]+(?:-[A-Za-z0-9.-]+)?$"));
    const QRegularExpression localePattern(QStringLiteral("^[a-z]{2}-[A-Z]{2}$"));
    if (config.applicationName.isEmpty() || !versionPattern.match(config.applicationVersion).hasMatch()
        || !localePattern.match(config.locale).hasMatch()
        || !(config.defaultTarget == QStringLiteral("desk") || config.defaultTarget == QStringLiteral("wall"))
        || !QSet<QString> {QStringLiteral("TRACE"), QStringLiteral("DEBUG"), QStringLiteral("INFO"),
                            QStringLiteral("WARN"), QStringLiteral("ERROR"), QStringLiteral("FATAL")}
                .contains(config.loggingLevel)
        || config.auditPath.isEmpty() || QDir::isAbsolutePath(config.auditPath)
        || config.auditPath.split('/').contains(QStringLiteral(".."))) {
        return invalidConfiguration(QStringLiteral("Configuration invalid; using safe defaults."));
    }

    if (!parsePositiveInteger(values.value(QStringLiteral("display.phone.width")), config.phoneWidth)
        || !parsePositiveInteger(values.value(QStringLiteral("display.phone.height")), config.phoneHeight)
        || !parsePositiveInteger(values.value(QStringLiteral("display.projection.width")), config.projectionWidth)
        || !parsePositiveInteger(values.value(QStringLiteral("display.projection.height")), config.projectionHeight)
        || !parsePositiveInteger(values.value(QStringLiteral("runtime.recent_task_limit")), config.recentTaskLimit)
        || !parsePositiveDouble(values.value(QStringLiteral("projection.rotation_speed")), config.rotationSpeed)
        || !parsePositiveDouble(values.value(QStringLiteral("projection.default_zoom")), config.defaultZoom)
        || !parseBoolean(values.value(QStringLiteral("display.projection.start_fullscreen")), config.startFullscreen)
        || !parseBoolean(values.value(QStringLiteral("projection.auto_rotate")), config.autoRotate)
        || !parseBoolean(values.value(QStringLiteral("authorization.simulated_authorized")), config.simulatedAuthorized)) {
        return invalidConfiguration(QStringLiteral("Configuration invalid; using safe defaults."));
    }

    ConfigurationLoadResult result;
    result.config = config;
    result.valid = true;
    result.schemaValidated = true;
    return result;
}

} // namespace astra::shell
