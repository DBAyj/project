#include "controllers/ShellController.h"

#include <QFile>
#include <QJsonObject>
#include <QJsonValue>
#include <QTemporaryDir>

#include <cassert>

int main()
{
    QTemporaryDir directory;
    astra::shell::SimulatorConfig config;
    astra::shell::ShellController controller(config, directory.filePath("audit/safety.jsonl"));
    controller.applySpatialState({{QStringLiteral("state"), QStringLiteral("READY")},
                                  {QStringLiteral("input_source"), QStringLiteral("SIMULATION")},
                                  {QStringLiteral("quality"), QStringLiteral("GOOD")},
                                  {QStringLiteral("warning"), QString()},
                                  {QStringLiteral("last_error_code"), QJsonValue::Null},
                                  {QStringLiteral("target"), QJsonObject {}}});
    controller.submit(QStringLiteral("开始投影"), QStringLiteral("PUBLIC"));
    assert(controller.projectionState() == QStringLiteral("ACTIVE"));

    controller.applySpatialState({{QStringLiteral("state"), QStringLiteral("LOST")},
                                  {QStringLiteral("input_source"), QStringLiteral("SIMULATION")},
                                  {QStringLiteral("quality"), QStringLiteral("UNUSABLE")},
                                  {QStringLiteral("warning"), QStringLiteral("Selected projection target was lost")},
                                  {QStringLiteral("last_error_code"), 3305},
                                  {QStringLiteral("target"), QJsonObject {{QStringLiteral("state"), QStringLiteral("LOST")}}}});
    assert(controller.projectionState() == QStringLiteral("IDLE"));
    assert(!controller.projectionModel()->modelVisible());
    assert(controller.systemState()->spatialStatus() == QStringLiteral("LOST"));
    assert(controller.lastErrorCode() == 3305);

    QFile audit(directory.filePath("audit/safety.jsonl"));
    assert(audit.open(QIODevice::ReadOnly));
    const QByteArray events = audit.readAll();
    assert(events.contains("spatial_safe_pause"));
    assert(events.contains("clear_sensitive_projection_content"));

    controller.submit(QStringLiteral("开始投影"), QStringLiteral("PUBLIC"));
    assert(controller.projectionState() == QStringLiteral("IDLE"));
    assert(controller.currentTaskStatus() == QStringLiteral("SAFE_PAUSED"));

    controller.applySpatialState({{QStringLiteral("state"), QStringLiteral("READY")},
                                  {QStringLiteral("input_source"), QStringLiteral("SIMULATION")},
                                  {QStringLiteral("quality"), QStringLiteral("GOOD")},
                                  {QStringLiteral("warning"), QString()},
                                  {QStringLiteral("last_error_code"), QJsonValue::Null},
                                  {QStringLiteral("target"), QJsonObject {}}});
    controller.submit(QStringLiteral("开始投影"), QStringLiteral("PUBLIC"));
    assert(controller.projectionState() == QStringLiteral("ACTIVE"));
    controller.applySpatialState({{QStringLiteral("state"), QStringLiteral("ERROR")},
                                  {QStringLiteral("input_source"), QStringLiteral("SIMULATION")},
                                  {QStringLiteral("quality"), QStringLiteral("UNUSABLE")},
                                  {QStringLiteral("warning"), QStringLiteral("Spatial service is unavailable")},
                                  {QStringLiteral("last_error_code"), 3504},
                                  {QStringLiteral("target"), QJsonObject {{QStringLiteral("state"), QStringLiteral("INVALID")}}}});
    assert(controller.projectionState() == QStringLiteral("IDLE"));
    assert(controller.lastErrorCode() == 3504);
}
