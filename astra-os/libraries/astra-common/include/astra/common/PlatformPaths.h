#pragma once

#include <QString>

namespace astra::common {

struct PlatformPaths {
    QString socketPath;
    QString statePath;
    QString auditPath;
    QString cacheDirectory;
    QString configurationDirectory;
    QString schemaDirectory;

    [[nodiscard]] static PlatformPaths forDevelopmentRoot(const QString &projectRoot);
    [[nodiscard]] static PlatformPaths forMacHost(const QString &homeDirectory);
    [[nodiscard]] static PlatformPaths forLinuxTarget();
    [[nodiscard]] static PlatformPaths forCurrentHost(const QString &developmentRoot = {});
};

} // namespace astra::common
