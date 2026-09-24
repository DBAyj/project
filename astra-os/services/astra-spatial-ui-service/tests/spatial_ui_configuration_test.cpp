#include "astra/spatial_ui/service/SpatialUIConfiguration.h"

#include <QDir>
#include <QFile>
#include <QTemporaryDir>
#include <QtGlobal>

namespace {

void require(bool condition, const char *message)
{
    if (!condition) qFatal("P5 configuration requirement failed: %s", message);
}

} // namespace

int main()
{
    const QDir project {QString::fromUtf8(ASTRA_TEST_PROJECT_ROOT)};
    const auto valid = astra::spatial_ui::service::SpatialUIConfiguration::load(
        project.filePath(QStringLiteral("config")), project.filePath(QStringLiteral("schemas")));
    require(valid.valid && valid.schemaValidated && !valid.usingSafeDefaults, "all five configurations validate");
    require(valid.options.maximumComponents == 500 && valid.options.maximumWindows == 20, "runtime limits loaded");
    require(valid.options.autosaveIntervalSeconds == 10, "autosave interval loaded");
    require(valid.options.statePersistence && valid.options.mouseEnabled && valid.options.keyboardEnabled,
            "state and input configuration loaded");
    require(valid.options.projectionTargetEnabled, "validated target context loaded");
    require(valid.options.maximumFixtureDisclosure == astra::common::PrivacyLevel::PrivateScreenOnly
                && valid.options.publicFixtureSubjectId == QStringLiteral("7abf7b1e-dc4f-4b7c-96cc-c3dd628e1ca9"),
            "policy-owned fixture classifications loaded");
    require(valid.options.defaultLayout == astra::ui::LayoutMode::Stack, "default layout loaded");
    require(valid.options.configurationStatus == QStringLiteral("VALID") && valid.options.configurationWarning.isEmpty(),
            "valid status exposed");

    const auto missing = astra::spatial_ui::service::SpatialUIConfiguration::load(
        project.filePath(QStringLiteral("missing-config")), project.filePath(QStringLiteral("schemas")));
    require(!missing.valid && missing.usingSafeDefaults, "missing configuration uses safe defaults");
    require(!missing.options.projectionTargetEnabled, "safe defaults disable external projection");

    QTemporaryDir temporary;
    require(temporary.isValid(), "temporary directory available");
    QDir temporaryRoot {temporary.path()};
    require(temporaryRoot.mkdir(QStringLiteral("config")), "temporary config directory created");
    const QDir temporaryConfig {temporaryRoot.filePath(QStringLiteral("config"))};
    const QStringList names {QStringLiteral("spatial-ui.yaml"), QStringLiteral("spatial-layout.yaml"),
                             QStringLiteral("spatial-input.yaml"), QStringLiteral("spatial-accessibility.yaml"),
                             QStringLiteral("spatial-theme.yaml")};
    for (const auto &name : names) {
        require(QFile::copy(project.filePath(QStringLiteral("config/") + name), temporaryConfig.filePath(name)),
                "configuration copied");
    }
    QFile invalidInput {temporaryConfig.filePath(QStringLiteral("spatial-input.yaml"))};
    require(invalidInput.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text), "invalid input opened");
    require(invalidInput.write("  unknown_input: true\n") > 0, "unknown field appended");
    invalidInput.close();
    const auto invalid = astra::spatial_ui::service::SpatialUIConfiguration::load(
        temporaryConfig.path(), project.filePath(QStringLiteral("schemas")));
    require(!invalid.valid && invalid.usingSafeDefaults, "unknown field is rejected at runtime");
    require(!invalid.options.projectionTargetEnabled, "invalid configuration fails closed");
}
