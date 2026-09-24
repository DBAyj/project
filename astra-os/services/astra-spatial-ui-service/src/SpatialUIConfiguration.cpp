#include "astra/spatial_ui/service/SpatialUIConfiguration.h"

#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QRegularExpression>
#include <QSet>

#include <cmath>
#include <optional>

namespace astra::spatial_ui::service {
namespace {

struct ConfigurationDocument {
    QString configurationFile;
    QString schemaFile;
};

const ConfigurationDocument kDocuments[] {
    {QStringLiteral("spatial-ui.yaml"), QStringLiteral("spatial-ui-config.schema.json")},
    {QStringLiteral("spatial-layout.yaml"), QStringLiteral("spatial-layout-config.schema.json")},
    {QStringLiteral("spatial-input.yaml"), QStringLiteral("spatial-input-config.schema.json")},
    {QStringLiteral("spatial-accessibility.yaml"), QStringLiteral("spatial-accessibility-config.schema.json")},
    {QStringLiteral("spatial-theme.yaml"), QStringLiteral("spatial-component-theme.schema.json")},
};

SpatialUIConfigurationLoadResult safeDefaults(const QString &warning)
{
    SpatialUIConfigurationLoadResult result;
    result.warning = warning;
    result.options.configurationStatus = QStringLiteral("SAFE_DEFAULTS");
    result.options.configurationWarning = warning;
    result.options.projectionTargetEnabled = false;
    return result;
}

void insertPath(QJsonObject &object, const QStringList &path, const QJsonValue &value)
{
    if (path.size() == 1) {
        object.insert(path.first(), value);
        return;
    }
    QJsonObject child = object.value(path.first()).toObject();
    insertPath(child, path.sliced(1), value);
    object.insert(path.first(), child);
}

std::optional<QJsonValue> yamlScalar(const QString &source)
{
    const QString value = source.trimmed();
    if (value == QStringLiteral("true")) return QJsonValue {true};
    if (value == QStringLiteral("false")) return QJsonValue {false};
    if (value == QStringLiteral("null") || value == QStringLiteral("~")) return QJsonValue {QJsonValue::Null};
    if (value.size() >= 2 && value.startsWith(QLatin1Char('"')) && value.endsWith(QLatin1Char('"'))) {
        QJsonParseError error;
        const QJsonDocument document = QJsonDocument::fromJson((QByteArrayLiteral("[") + value.toUtf8() + QByteArrayLiteral("]")), &error);
        if (error.error != QJsonParseError::NoError || !document.isArray() || document.array().size() != 1
            || !document.array().first().isString()) {
            return std::nullopt;
        }
        return document.array().first();
    }
    if (value.size() >= 2 && value.startsWith(QLatin1Char('\'')) && value.endsWith(QLatin1Char('\''))) {
        QString unquoted = value.mid(1, value.size() - 2);
        unquoted.replace(QStringLiteral("''"), QStringLiteral("'"));
        return QJsonValue {unquoted};
    }
    static const QRegularExpression integerPattern {QStringLiteral("^-?(0|[1-9][0-9]*)$")};
    if (integerPattern.match(value).hasMatch()) {
        bool ok = false;
        const qlonglong parsed = value.toLongLong(&ok);
        if (ok) return QJsonValue {parsed};
    }
    static const QRegularExpression numberPattern {QStringLiteral("^-?(?:0|[1-9][0-9]*)\\.[0-9]+$")};
    if (numberPattern.match(value).hasMatch()) {
        bool ok = false;
        const double parsed = value.toDouble(&ok);
        if (ok && std::isfinite(parsed)) return QJsonValue {parsed};
    }
    static const QRegularExpression bareStringPattern {QStringLiteral("^[A-Za-z0-9_./-]+$")};
    return bareStringPattern.match(value).hasMatch() ? std::optional<QJsonValue> {QJsonValue {value}} : std::nullopt;
}

std::optional<QJsonObject> parseYamlObject(const QByteArray &source)
{
    QJsonObject result;
    QStringList hierarchy;
    QSet<QString> paths;
    static const QRegularExpression entryPattern {QStringLiteral("^([A-Za-z][A-Za-z0-9_]*)\\s*:(.*)$")};
    for (const QString &rawLine : QString::fromUtf8(source).split(QLatin1Char('\n'))) {
        if (rawLine.trimmed().isEmpty() || rawLine.trimmed().startsWith(QLatin1Char('#'))) continue;
        if (rawLine.contains(QLatin1Char('\t'))) return std::nullopt;
        int indentation = 0;
        while (indentation < rawLine.size() && rawLine.at(indentation) == QLatin1Char(' ')) ++indentation;
        if (indentation % 2 != 0) return std::nullopt;
        const int level = indentation / 2;
        if (level > hierarchy.size()) return std::nullopt;
        while (hierarchy.size() > level) hierarchy.removeLast();
        const auto match = entryPattern.match(rawLine.mid(indentation));
        if (!match.hasMatch()) return std::nullopt;
        const QString key = match.captured(1);
        const QString rawValue = match.captured(2).trimmed();
        QStringList path = hierarchy;
        path.append(key);
        const QString flattened = path.join(QLatin1Char('.'));
        if (paths.contains(flattened)) return std::nullopt;
        paths.insert(flattened);
        if (rawValue.isEmpty()) {
            insertPath(result, path, QJsonObject {});
            hierarchy.append(key);
            continue;
        }
        const auto scalar = yamlScalar(rawValue);
        if (!scalar) return std::nullopt;
        insertPath(result, path, *scalar);
    }
    return result;
}

bool matchesType(const QJsonValue &value, const QString &type)
{
    if (type == QStringLiteral("object")) return value.isObject();
    if (type == QStringLiteral("array")) return value.isArray();
    if (type == QStringLiteral("string")) return value.isString();
    if (type == QStringLiteral("boolean")) return value.isBool();
    if (type == QStringLiteral("null")) return value.isNull();
    if (type == QStringLiteral("number")) return value.isDouble();
    if (type == QStringLiteral("integer")) {
        return value.isDouble() && std::floor(value.toDouble()) == value.toDouble();
    }
    return false;
}

std::optional<QJsonObject> resolveReference(const QJsonObject &root, const QString &reference)
{
    if (!reference.startsWith(QStringLiteral("#/"))) return std::nullopt;
    QJsonValue value {root};
    for (QString segment : reference.mid(2).split(QLatin1Char('/'))) {
        segment.replace(QStringLiteral("~1"), QStringLiteral("/"));
        segment.replace(QStringLiteral("~0"), QStringLiteral("~"));
        if (!value.isObject() || !value.toObject().contains(segment)) return std::nullopt;
        value = value.toObject().value(segment);
    }
    return value.isObject() ? std::optional<QJsonObject> {value.toObject()} : std::nullopt;
}

bool validateJson(const QJsonValue &value, const QJsonObject &schema, const QJsonObject &root)
{
    if (schema.contains(QStringLiteral("$ref"))) {
        const auto referenced = resolveReference(root, schema.value(QStringLiteral("$ref")).toString());
        return referenced && validateJson(value, *referenced, root);
    }
    if (schema.contains(QStringLiteral("type"))) {
        bool typeMatched = false;
        const QJsonValue type = schema.value(QStringLiteral("type"));
        if (type.isString()) typeMatched = matchesType(value, type.toString());
        if (type.isArray()) {
            for (const auto &candidate : type.toArray()) {
                if (candidate.isString() && matchesType(value, candidate.toString())) typeMatched = true;
            }
        }
        if (!typeMatched) return false;
    }
    if (schema.contains(QStringLiteral("const")) && value != schema.value(QStringLiteral("const"))) return false;
    if (schema.contains(QStringLiteral("enum"))) {
        bool found = false;
        for (const auto &candidate : schema.value(QStringLiteral("enum")).toArray()) {
            if (candidate == value) found = true;
        }
        if (!found) return false;
    }
    if (value.isObject()) {
        const QJsonObject object = value.toObject();
        const QJsonObject properties = schema.value(QStringLiteral("properties")).toObject();
        for (const auto &required : schema.value(QStringLiteral("required")).toArray()) {
            if (!required.isString() || !object.contains(required.toString())) return false;
        }
        if (schema.value(QStringLiteral("additionalProperties")).isBool()
            && !schema.value(QStringLiteral("additionalProperties")).toBool()) {
            for (auto iterator = object.begin(); iterator != object.end(); ++iterator) {
                if (!properties.contains(iterator.key())) return false;
            }
        }
        for (auto iterator = object.begin(); iterator != object.end(); ++iterator) {
            if (properties.contains(iterator.key())
                && !validateJson(iterator.value(), properties.value(iterator.key()).toObject(), root)) {
                return false;
            }
        }
    }
    if (value.isDouble()) {
        const double number = value.toDouble();
        if (schema.contains(QStringLiteral("minimum")) && number < schema.value(QStringLiteral("minimum")).toDouble()) return false;
        if (schema.contains(QStringLiteral("maximum")) && number > schema.value(QStringLiteral("maximum")).toDouble()) return false;
    }
    if (value.isString() && schema.contains(QStringLiteral("pattern"))) {
        const QRegularExpression pattern {schema.value(QStringLiteral("pattern")).toString()};
        if (!pattern.isValid() || !pattern.match(value.toString()).hasMatch()) return false;
    }
    return true;
}

std::optional<QJsonObject> loadAndValidate(const QString &configurationPath, const QString &schemaPath)
{
    QFile configurationFile {configurationPath};
    QFile schemaFile {schemaPath};
    if (!configurationFile.open(QIODevice::ReadOnly | QIODevice::Text) || !schemaFile.open(QIODevice::ReadOnly)) return std::nullopt;
    const auto configuration = parseYamlObject(configurationFile.readAll());
    QJsonParseError error;
    const QJsonDocument schemaDocument = QJsonDocument::fromJson(schemaFile.readAll(), &error);
    if (!configuration || error.error != QJsonParseError::NoError || !schemaDocument.isObject()) return std::nullopt;
    const QJsonObject schema = schemaDocument.object();
    if (schema.value(QStringLiteral("$schema")).toString() != QStringLiteral("https://json-schema.org/draft/2020-12/schema")
        || !validateJson(*configuration, schema, schema)) {
        return std::nullopt;
    }
    return configuration;
}

std::optional<astra::ui::LayoutMode> parseLayoutMode(const QString &value)
{
    if (value == QStringLiteral("STACK")) return astra::ui::LayoutMode::Stack;
    if (value == QStringLiteral("GRID")) return astra::ui::LayoutMode::Grid;
    if (value == QStringLiteral("RADIAL")) return astra::ui::LayoutMode::Radial;
    if (value == QStringLiteral("FREEFORM")) return astra::ui::LayoutMode::Freeform;
    if (value == QStringLiteral("ANCHOR_RELATIVE")) return astra::ui::LayoutMode::AnchorRelative;
    return std::nullopt;
}

} // namespace

SpatialUIConfigurationLoadResult SpatialUIConfiguration::load(const QString &configurationDirectory,
                                                              const QString &schemaDirectory)
{
    if (configurationDirectory.isEmpty() || schemaDirectory.isEmpty()) {
        return safeDefaults(QStringLiteral("Spatial UI configuration paths are unavailable; projection is disabled."));
    }
    const QDir configurations {configurationDirectory};
    const QDir schemas {schemaDirectory};
    QHash<QString, QJsonObject> documents;
    for (const auto &document : kDocuments) {
        const auto value = loadAndValidate(configurations.filePath(document.configurationFile), schemas.filePath(document.schemaFile));
        if (!value) {
            return safeDefaults(QStringLiteral("Spatial UI configuration or Schema validation failed for %1; projection is disabled.")
                                    .arg(document.configurationFile));
        }
        documents.insert(document.configurationFile, *value);
    }

    SpatialUIConfigurationLoadResult result;
    const QJsonObject ui = documents.value(QStringLiteral("spatial-ui.yaml"));
    const QJsonObject runtime = ui.value(QStringLiteral("runtime")).toObject();
    const QJsonObject display = ui.value(QStringLiteral("display")).toObject();
    const QJsonObject privacy = ui.value(QStringLiteral("privacy")).toObject();
    const QJsonObject layout = documents.value(QStringLiteral("spatial-layout.yaml")).value(QStringLiteral("layout")).toObject();
    const QJsonObject input = documents.value(QStringLiteral("spatial-input.yaml")).value(QStringLiteral("input")).toObject();
    const QJsonObject accessibility = documents.value(QStringLiteral("spatial-accessibility.yaml"))
                                          .value(QStringLiteral("accessibility")).toObject();
    const auto defaultLayout = parseLayoutMode(layout.value(QStringLiteral("default_mode")).toString());
    if (!defaultLayout) return safeDefaults(QStringLiteral("Spatial UI default layout is invalid; projection is disabled."));

    result.options.maximumComponents = runtime.value(QStringLiteral("maximum_components")).toInt();
    result.options.maximumWindows = runtime.value(QStringLiteral("maximum_windows")).toInt();
    result.options.statePersistence = runtime.value(QStringLiteral("state_persistence")).toBool();
    result.options.autosaveIntervalSeconds = runtime.value(QStringLiteral("autosave_interval_seconds")).toInt();
    result.options.projectionTargetEnabled = display.value(QStringLiteral("projection_target_enabled")).toBool();
    const auto defaultPrivacy = astra::common::privacyLevelFromString(
        privacy.value(QStringLiteral("default_level")).toString().toStdString());
    if (!defaultPrivacy) return safeDefaults(QStringLiteral("Spatial UI default privacy level is invalid; projection is disabled."));
    result.options.maximumFixtureDisclosure = *defaultPrivacy;
    result.options.publicFixtureSubjectId = privacy.value(QStringLiteral("public_fixture_subject_id")).toString();
    result.options.defaultLayout = *defaultLayout;
    result.options.layoutSpacing = layout.value(QStringLiteral("spacing")).toDouble();
    result.options.layoutMargin = layout.value(QStringLiteral("margin")).toDouble();
    result.options.mouseEnabled = input.value(QStringLiteral("mouse_enabled")).toBool();
    result.options.keyboardEnabled = input.value(QStringLiteral("keyboard_enabled")).toBool();
    result.options.touchEnabled = input.value(QStringLiteral("touch_enabled")).toBool();
    result.options.simulatedGestureEnabled = input.value(QStringLiteral("simulated_gesture_enabled")).toBool();
    result.options.accessibilityEnabled = accessibility.value(QStringLiteral("enabled")).toBool();
    result.options.keyboardNavigationEnabled = accessibility.value(QStringLiteral("keyboard_navigation")).toBool();
    result.options.visibleFocus = accessibility.value(QStringLiteral("visible_focus")).toBool();
    result.options.reduceMotion = accessibility.value(QStringLiteral("reduce_motion")).toBool();
    result.options.highContrast = accessibility.value(QStringLiteral("high_contrast")).toBool();
    result.options.configurationStatus = QStringLiteral("VALID");
    result.options.configurationWarning.clear();
    result.valid = true;
    result.schemaValidated = true;
    result.usingSafeDefaults = false;
    return result;
}

} // namespace astra::spatial_ui::service
