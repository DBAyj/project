#include "astra/common/PlatformPaths.h"

#include <QDir>

namespace astra::common {

PlatformPaths PlatformPaths::forDevelopmentRoot(const QString &projectRoot)
{
    const QDir root {QDir(projectRoot).absolutePath()};
    return {root.filePath(QStringLiteral("runtime/spatial-ui/sockets/astra-spatial-ui.sock")),
            root.filePath(QStringLiteral("runtime/spatial-ui/state/ui-state.json")),
            root.filePath(QStringLiteral("runtime/spatial-ui/audit/spatial-ui-audit.jsonl")),
            root.filePath(QStringLiteral("runtime/spatial-ui/cache")),
            root.filePath(QStringLiteral("config")), root.filePath(QStringLiteral("schemas"))};
}

PlatformPaths PlatformPaths::forMacHost(const QString &homeDirectory)
{
    const QDir home {QDir(homeDirectory).absolutePath()};
    return {home.filePath(QStringLiteral("Library/Application Support/AstraOS/run/astra-spatial-ui.sock")),
            home.filePath(QStringLiteral("Library/Application Support/AstraOS/state/ui-state.json")),
            home.filePath(QStringLiteral("Library/Logs/AstraOS/spatial-ui-audit.jsonl")),
            home.filePath(QStringLiteral("Library/Caches/AstraOS/spatial-ui")),
            home.filePath(QStringLiteral("Library/Application Support/AstraOS/config")),
            home.filePath(QStringLiteral("Library/Application Support/AstraOS/schemas"))};
}

PlatformPaths PlatformPaths::forLinuxTarget()
{
    return {QStringLiteral("/run/astra-os/astra-spatial-ui.sock"),
            QStringLiteral("/var/lib/astra-os/spatial-ui/ui-state.json"),
            QStringLiteral("/var/log/astra-os/spatial-ui-audit.jsonl"),
            QStringLiteral("/var/cache/astra-os/spatial-ui"), QStringLiteral("/etc/astra-os"),
            QStringLiteral("/usr/share/astra-os/schemas")};
}

PlatformPaths PlatformPaths::forCurrentHost(const QString &developmentRoot)
{
    if (!developmentRoot.isEmpty()) return forDevelopmentRoot(developmentRoot);
#if defined(Q_OS_LINUX)
    return forLinuxTarget();
#else
    return forMacHost(QDir::homePath());
#endif
}

} // namespace astra::common
