#pragma once

#include <QString>

namespace astra::shell {

struct SimulatorConfig {
    QString applicationName {QStringLiteral("AstraOS Simulator")};
    QString applicationVersion {QStringLiteral("0.1.0-alpha.1")};
    QString locale {QStringLiteral("zh-CN")};
    int phoneWidth {430};
    int phoneHeight {860};
    int projectionWidth {1280};
    int projectionHeight {720};
    bool startFullscreen {false};
    bool autoRotate {true};
    bool simulatedAuthorized {false};
    int recentTaskLimit {10};
    QString defaultTarget {QStringLiteral("desk")};
    double rotationSpeed {1.0};
    double defaultZoom {1.0};
    QString loggingLevel {QStringLiteral("INFO")};
    QString auditPath {QStringLiteral("runtime/audit/audit.jsonl")};
};

struct ConfigurationLoadResult {
    SimulatorConfig config;
    bool valid {false};
    QString warning;
    int errorCode {0};
    bool schemaValidated {false};
    bool usingFallbackConfiguration {false};
};

class ConfigurationService {
public:
    static ConfigurationLoadResult load(const QString &path, const QString &schemaPath = {});
};

} // namespace astra::shell
