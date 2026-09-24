#include "services/ConfigurationService.h"

#include <QTemporaryDir>
#include <QFile>
#include <cassert>

int main()
{
    QTemporaryDir directory;
    QFile validFile(directory.filePath("valid.yaml"));
    assert(validFile.open(QIODevice::WriteOnly | QIODevice::Text));
    validFile.write("application:\n  name: \"AstraOS Simulator\"\n  version: \"0.1.0-alpha.1\"\n  locale: \"zh-CN\"\ndisplay:\n  phone:\n    width: 430\n    height: 860\n  projection:\n    width: 1280\n    height: 720\n    start_fullscreen: false\nprojection:\n  default_target: \"desk\"\n  auto_rotate: true\n  rotation_speed: 1.0\n  default_zoom: 1.0\nauthorization:\n  simulated_authorized: false\nlogging:\n  level: \"INFO\"\n  audit_path: \"runtime/audit/audit.jsonl\"\nruntime:\n  recent_task_limit: 10\n");
    validFile.close();
    const auto valid = astra::shell::ConfigurationService::load(validFile.fileName(), QStringLiteral(ASTRA_SCHEMA_PATH));
    assert(valid.valid);
    assert(valid.config.phoneWidth == 430);
    assert(valid.config.projectionWidth == 1280);
    assert(valid.config.startFullscreen == false);
    assert(valid.config.defaultZoom == 1.0);
    assert(valid.config.auditPath == QStringLiteral("runtime/audit/audit.jsonl"));
    assert(valid.config.recentTaskLimit == 10);
    assert(valid.schemaValidated);

    const auto missingSchema = astra::shell::ConfigurationService::load(validFile.fileName(), directory.filePath("missing-schema.json"));
    assert(!missingSchema.valid);
    assert(missingSchema.usingFallbackConfiguration);

    QFile unknownFile(directory.filePath("unknown.yaml"));
    assert(unknownFile.open(QIODevice::WriteOnly | QIODevice::Text));
    unknownFile.write("application:\n  name: AstraOS Simulator\n  version: 0.1.0-alpha.1\n  locale: zh-CN\n  extra: rejected\ndisplay:\n  phone:\n    width: 430\n    height: 860\n  projection:\n    width: 1280\n    height: 720\n    start_fullscreen: false\nprojection:\n  default_target: desk\n  auto_rotate: true\n  rotation_speed: 1.0\n  default_zoom: 1.0\nauthorization:\n  simulated_authorized: false\nlogging:\n  level: INFO\n  audit_path: runtime/audit/audit.jsonl\nruntime:\n  recent_task_limit: 10\n");
    unknownFile.close();
    const auto unknown = astra::shell::ConfigurationService::load(unknownFile.fileName());
    assert(!unknown.valid);
    assert(unknown.errorCode == 1002);
    assert(unknown.warning.contains("invalid"));

    QFile versionFile(directory.filePath("bad-version.yaml"));
    assert(versionFile.open(QIODevice::WriteOnly | QIODevice::Text));
    versionFile.write("application:\n  name: AstraOS Simulator\n  version: invalid\n  locale: zh-CN\ndisplay:\n  phone:\n    width: 430\n    height: 860\n  projection:\n    width: 1280\n    height: 720\n    start_fullscreen: false\nprojection:\n  default_target: desk\n  auto_rotate: true\n  rotation_speed: 1.0\n  default_zoom: 1.0\nauthorization:\n  simulated_authorized: false\nlogging:\n  level: INFO\n  audit_path: runtime/audit/audit.jsonl\nruntime:\n  recent_task_limit: 10\n");
    versionFile.close();
    const auto badVersion = astra::shell::ConfigurationService::load(versionFile.fileName());
    assert(!badVersion.valid);
    assert(badVersion.usingFallbackConfiguration);

    const auto invalid = astra::shell::ConfigurationService::load(directory.filePath("missing.yaml"));
    assert(!invalid.valid);
    assert(invalid.errorCode == 1002);
    assert(invalid.config.phoneWidth == 430);
    assert(invalid.warning.contains("safe defaults"));
}
