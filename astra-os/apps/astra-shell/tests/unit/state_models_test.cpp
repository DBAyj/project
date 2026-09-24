#include "models/ProjectionStateModel.h"
#include "models/SystemStateModel.h"
#include "models/TaskModel.h"

#include <cassert>

int main()
{
    astra::shell::SystemStateModel system;
    system.setConfigurationStatus(QStringLiteral("VALID"));
    system.setConfigurationDiagnostics({QStringLiteral("Configuration invalid; using safe defaults.")}, true);
    system.setAuditStatus(QStringLiteral("READY"));
    system.setProjectionStatus(QStringLiteral("ACTIVE"));
    assert(system.configurationStatus() == QStringLiteral("VALID"));
    assert(system.configurationWarningCount() == 1);
    assert(system.usingFallbackConfiguration());
    assert(system.auditStatus() == QStringLiteral("READY"));
    assert(system.projectionStatus() == QStringLiteral("ACTIVE"));
    assert(!system.cpuArchitecture().isEmpty());

    astra::shell::TaskModel tasks(2);
    tasks.append({QStringLiteral("显示设备模型"), QStringLiteral("project_3d_model"), QStringLiteral("desk"),
                  QStringLiteral("PUBLIC"), QStringLiteral("ACTIVE"), QString(), QStringLiteral("request-1"),
                  QStringLiteral("2026-07-14T08:30:00Z")});
    tasks.append({QStringLiteral("停止投影"), QStringLiteral("stop_projection"), QStringLiteral("desk"),
                  QStringLiteral("PUBLIC"), QStringLiteral("IDLE"), QString(), QStringLiteral("request-2"),
                  QStringLiteral("2026-07-14T08:31:00Z")});
    tasks.append({QStringLiteral("未知任务"), QStringLiteral("unknown"), QStringLiteral("desk"),
                  QStringLiteral("PUBLIC"), QStringLiteral("FAILED"), QStringLiteral("无法识别任务"),
                  QStringLiteral("request-3"), QStringLiteral("2026-07-14T08:32:00Z")});
    assert(tasks.rowCount() == 2);
    assert(tasks.data(tasks.index(0, 0), astra::shell::TaskModel::RawTextRole) == QStringLiteral("停止投影"));
    assert(tasks.data(tasks.index(1, 0), astra::shell::TaskModel::ErrorMessageRole) == QStringLiteral("无法识别任务"));

    astra::shell::ProjectionStateModel projection;
    projection.applySession(QStringLiteral("ACTIVE"), QStringLiteral("session-1"), QStringLiteral("ROOM_ONLY"), QStringLiteral("desk"));
    assert(projection.modelVisible());
    assert(projection.sessionId() == QStringLiteral("session-1"));
    projection.setRotationPaused(true);
    projection.setView(22.0, 1.4);
    assert(projection.rotationPaused());
    assert(projection.sceneRotation() == 22.0);
    assert(projection.zoom() == 1.4);
    projection.clear();
    assert(projection.state() == QStringLiteral("IDLE"));
    assert(!projection.modelVisible());
    assert(projection.sessionId().isEmpty());
}
