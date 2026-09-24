#include "astra/common/PlatformPaths.h"

#include <cassert>

int main()
{
    using astra::common::PlatformPaths;

    const auto development = PlatformPaths::forDevelopmentRoot(QStringLiteral("/workspace/astra-os"));
    assert(development.socketPath == QStringLiteral("/workspace/astra-os/runtime/spatial-ui/sockets/astra-spatial-ui.sock"));
    assert(development.statePath == QStringLiteral("/workspace/astra-os/runtime/spatial-ui/state/ui-state.json"));

    const auto mac = PlatformPaths::forMacHost(QStringLiteral("/Users/test"));
    assert(mac.socketPath == QStringLiteral("/Users/test/Library/Application Support/AstraOS/run/astra-spatial-ui.sock"));
    assert(mac.auditPath == QStringLiteral("/Users/test/Library/Logs/AstraOS/spatial-ui-audit.jsonl"));

    const auto linux = PlatformPaths::forLinuxTarget();
    assert(linux.socketPath == QStringLiteral("/run/astra-os/astra-spatial-ui.sock"));
    assert(linux.statePath == QStringLiteral("/var/lib/astra-os/spatial-ui/ui-state.json"));
    assert(linux.auditPath == QStringLiteral("/var/log/astra-os/spatial-ui-audit.jsonl"));
}
