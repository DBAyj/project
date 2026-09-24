#include "services/IntentSimulator.h"

#include <cassert>

int main()
{
    astra::shell::IntentSimulator simulator;
    const auto project = simulator.parse("把设备模型投到桌面上", "PUBLIC");
    assert(project.intent == "project_3d_model");
    assert(project.privacyLevel == "PUBLIC");
    assert(project.parameters.value("model_id") == "demo-device");
    assert(project.timestamp.endsWith('Z'));
    assert(simulator.parse("停止投影").intent == "stop_projection");
    assert(simulator.parse("project the device model").intent == "project_3d_model");
    assert(simulator.parse("开始投影").intent == "project_3d_model");
    assert(simulator.parse("旋转模型").intent == "rotate_model");
    assert(simulator.parse("暂停旋转").intent == "pause_rotation");
    assert(simulator.parse("继续旋转").intent == "resume_rotation");
    assert(simulator.parse("重置视角").intent == "reset_view");
    assert(simulator.parse("进入全屏").intent == "enter_fullscreen");
    assert(simulator.parse("退出全屏").intent == "exit_fullscreen");
    assert(simulator.parse("查看系统状态").intent == "show_system_status");
    const auto unknown = simulator.parse("帮我订一张去上海的机票");
    assert(unknown.intent == "unknown");
    assert(unknown.confidence == 0.0);
}
