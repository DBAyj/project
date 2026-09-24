#include "services/IntentSimulator.h"

#include "astra/common/Identifiers.h"

namespace astra::shell {
SimulatedIntent IntentSimulator::parse(const QString &text, const QString &privacyLevel) const
{
    SimulatedIntent result {
        QString::fromStdString(astra::common::newUuid()),
        QString::fromStdString(astra::common::newUuid()),
        text.trimmed(),
        QStringLiteral("unknown"),
        0.0,
        QStringLiteral("desk"),
        privacyLevel,
        {},
        QString::fromStdString(astra::common::utcTimestamp()),
    };
    const QString normalized = result.rawText.toLower();
    if (normalized.contains(QStringLiteral("隐藏投影内容")) || normalized.contains(QStringLiteral("清空外部画面"))
        || normalized.contains(QStringLiteral("hide projection content"))) {
        result.intent = QStringLiteral("hide_projection_content");
        result.confidence = 1.0;
    } else if (normalized.contains(QStringLiteral("停止投影")) || normalized.contains(QStringLiteral("关闭投影")) || normalized.contains(QStringLiteral("stop projection"))) {
        result.intent = QStringLiteral("stop_projection");
        result.confidence = 1.0;
    } else if (normalized.contains(QStringLiteral("暂停旋转")) || normalized.contains(QStringLiteral("pause rotation"))) {
        result.intent = QStringLiteral("pause_rotation");
        result.confidence = 1.0;
    } else if (normalized.contains(QStringLiteral("继续旋转")) || normalized.contains(QStringLiteral("resume rotation"))) {
        result.intent = QStringLiteral("resume_rotation");
        result.confidence = 1.0;
    } else if (normalized.contains(QStringLiteral("重置视角")) || normalized.contains(QStringLiteral("reset view"))) {
        result.intent = QStringLiteral("reset_view");
        result.confidence = 1.0;
    } else if (normalized.contains(QStringLiteral("进入全屏")) || normalized.contains(QStringLiteral("enter fullscreen"))) {
        result.intent = QStringLiteral("enter_fullscreen");
        result.confidence = 1.0;
    } else if (normalized.contains(QStringLiteral("退出全屏")) || normalized.contains(QStringLiteral("exit fullscreen"))) {
        result.intent = QStringLiteral("exit_fullscreen");
        result.confidence = 1.0;
    } else if (normalized.contains(QStringLiteral("查看系统状态")) || normalized.contains(QStringLiteral("show system status"))) {
        result.intent = QStringLiteral("show_system_status");
        result.confidence = 1.0;
    } else if (normalized.contains(QStringLiteral("旋转模型")) || normalized.contains(QStringLiteral("rotate model"))) {
        result.intent = QStringLiteral("rotate_model");
        result.confidence = 1.0;
    } else if (normalized.contains(QStringLiteral("设备模型")) || normalized.contains(QStringLiteral("开始投影")) || normalized.contains(QStringLiteral("显示设备模型")) || normalized.contains(QStringLiteral("project the device model")) || normalized.contains(QStringLiteral("start projection"))) {
        result.intent = QStringLiteral("project_3d_model");
        result.confidence = 1.0;
        if (normalized.contains(QStringLiteral("墙"))) result.targetSpace = QStringLiteral("wall");
        result.parameters.insert(QStringLiteral("model_id"), QStringLiteral("demo-device"));
    }
    return result;
}
} // namespace astra::shell
