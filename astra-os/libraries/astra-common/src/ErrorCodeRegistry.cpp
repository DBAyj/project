#include "astra/common/ErrorCodeRegistry.h"

#include <QJsonArray>
#include <QJsonObject>

namespace astra::common {
namespace {

const QVector<ErrorDescriptor> &descriptors()
{
    static const QVector<ErrorDescriptor> values {
        {1002, "CONFIGURATION_INVALID", "配置文件无效", "Configuration is invalid", "astra-settings", ErrorSeverity::Error, false, true},
        {1003, "APPLICATION_INITIALIZATION_FAILED", "应用初始化失败", "Application initialization failed", "astra-shell", ErrorSeverity::Critical, false, true},
        {2001, "INTENT_UNRECOGNIZED", "意图无法识别", "Intent cannot be recognized", "astra-intent", ErrorSeverity::Warning, true, true},
        {2002, "LOCAL_INTENT_MODEL_UNAVAILABLE", "本地意图模型不可用", "Local intent model is unavailable", "astra-intent", ErrorSeverity::Error, true, true},
        {2003, "INTENT_SERVICE_TIMEOUT", "意图服务超时", "Intent service timed out", "astra-intent", ErrorSeverity::Error, true, true},
        {2004, "INTENT_SERVICE_INTERNAL_ERROR", "意图服务内部错误", "Intent service internal error", "astra-intent", ErrorSeverity::Critical, true, true},
        {2005, "REQUIRED_INTENT_PARAMETER_MISSING", "缺少必要参数", "Required intent parameter is missing", "astra-intent", ErrorSeverity::Warning, true, true},
        {2006, "CONFLICTING_INTENT_CANDIDATES", "存在多个冲突意图", "Multiple intent candidates conflict", "astra-intent", ErrorSeverity::Warning, true, true},
        {2007, "INTENT_CONFIDENCE_INSUFFICIENT", "意图置信度不足", "Intent confidence is insufficient", "astra-intent", ErrorSeverity::Warning, true, true},
        {2008, "INTENT_LOCALE_UNSUPPORTED", "不支持的语言", "Intent locale is unsupported", "astra-intent", ErrorSeverity::Warning, false, true},
        {2009, "INTENT_REQUEST_INVALID", "无效意图请求", "Intent request is invalid", "astra-intent", ErrorSeverity::Warning, false, true},
        {2010, "INTENT_CONFIGURATION_INVALID", "意图配置无效", "Intent configuration is invalid", "astra-intent", ErrorSeverity::Error, false, true},
        {2101, "USER_CONFIRMATION_REQUIRED", "需要用户确认", "User confirmation is required", "astra-intent", ErrorSeverity::Warning, true, true},
        {2102, "CONFIRMATION_EXPIRED", "确认已过期", "Confirmation has expired", "astra-intent", ErrorSeverity::Warning, true, true},
        {2103, "CONFIRMATION_REJECTED", "确认被拒绝", "Confirmation was rejected", "astra-intent", ErrorSeverity::Warning, false, true},
        {2104, "CLARIFICATION_REQUIRED", "需要补充信息", "Additional information is required", "astra-intent", ErrorSeverity::Warning, true, true},
        {2201, "PROMPT_INJECTION_RISK_DETECTED", "检测到提示词注入风险", "Prompt injection risk was detected", "astra-security", ErrorSeverity::Error, false, true},
        {2202, "RESTRICTED_INPUT_CONTENT", "输入包含受限制内容", "Input contains restricted content", "astra-security", ErrorSeverity::Error, false, true},
        {2203, "INPUT_LENGTH_EXCEEDED", "输入长度超限", "Input length exceeds the configured limit", "astra-security", ErrorSeverity::Warning, true, true},
        {2204, "SENSITIVE_DATA_PROCESSING_FAILED", "敏感数据处理失败", "Sensitive data processing failed", "astra-security", ErrorSeverity::Error, true, true},
        {2301, "RULE_ENGINE_UNAVAILABLE", "规则引擎不可用", "Rule engine is unavailable", "astra-intent", ErrorSeverity::Error, true, true},
        {2302, "LOCAL_MODEL_ADAPTER_UNAVAILABLE", "本地模型适配器不可用", "Local model adapter is unavailable", "astra-intent", ErrorSeverity::Error, true, true},
        {2303, "CANDIDATE_MERGE_FAILED", "候选意图融合失败", "Intent candidate merge failed", "astra-intent", ErrorSeverity::Error, true, true},
        {2304, "SLOT_EXTRACTION_FAILED", "槽位提取失败", "Intent slot extraction failed", "astra-intent", ErrorSeverity::Error, true, true},
        {3001, "SPATIAL_SERVICE_NOT_STARTED", "空间服务未启动", "Spatial service is not started", "astra-spatial", ErrorSeverity::Warning, true, true},
        {3002, "SPATIAL_SERVICE_START_FAILED", "空间服务启动失败", "Spatial service failed to start", "astra-spatial", ErrorSeverity::Critical, true, true},
        {3003, "INVALID_SPATIAL_TRANSITION", "空间服务状态转换非法", "Spatial service state transition is invalid", "astra-spatial", ErrorSeverity::Error, false, true},
        {3004, "SPATIAL_INPUT_UNAVAILABLE", "输入源不可用", "Spatial input source is unavailable", "astra-spatial", ErrorSeverity::Error, true, true},
        {3005, "CAMERA_UNAVAILABLE", "摄像头不可用", "Camera is unavailable", "astra-spatial", ErrorSeverity::Error, true, true},
        {3006, "CAMERA_PERMISSION_DENIED", "摄像头权限被拒绝", "Camera permission was denied", "astra-spatial", ErrorSeverity::Warning, true, true},
        {3007, "CAMERA_DISCONNECTED", "摄像头设备断开", "Camera device disconnected", "astra-spatial", ErrorSeverity::Error, true, true},
        {3008, "SPATIAL_FRAME_INVALID", "图像帧无效", "Spatial image frame is invalid", "astra-spatial", ErrorSeverity::Error, true, true},
        {3009, "VIDEO_INPUT_UNAVAILABLE", "视频输入不可用", "Video input is unavailable", "astra-spatial", ErrorSeverity::Error, true, true},
        {3101, "SURFACE_NOT_DETECTED", "未检测到有效表面", "No valid surface was detected", "astra-spatial", ErrorSeverity::Warning, true, true},
        {3102, "SURFACE_QUALITY_INSUFFICIENT", "表面候选质量不足", "Surface candidate quality is insufficient", "astra-spatial", ErrorSeverity::Warning, true, true},
        {3103, "SURFACE_CANDIDATE_MISSING", "表面候选不存在", "Surface candidate does not exist", "astra-spatial", ErrorSeverity::Warning, true, true},
        {3104, "SURFACE_CANDIDATE_INVALIDATED", "表面候选已失效", "Surface candidate is no longer valid", "astra-spatial", ErrorSeverity::Error, true, true},
        {3105, "SURFACE_CLASSIFICATION_FAILED", "表面分类失败", "Surface classification failed", "astra-spatial", ErrorSeverity::Error, true, true},
        {3201, "CALIBRATION_NOT_STARTED", "标定未开始", "Calibration has not started", "astra-spatial", ErrorSeverity::Warning, true, true},
        {3202, "CALIBRATION_POINT_INVALID", "标定点无效", "Calibration point is invalid", "astra-spatial", ErrorSeverity::Warning, true, true},
        {3203, "CALIBRATION_QUADRILATERAL_SELF_INTERSECTING", "标定四边形自相交", "Calibration quadrilateral self-intersects", "astra-spatial", ErrorSeverity::Warning, true, true},
        {3204, "CALIBRATION_AREA_TOO_SMALL", "标定区域过小", "Calibration area is too small", "astra-spatial", ErrorSeverity::Warning, true, true},
        {3205, "HOMOGRAPHY_CALCULATION_FAILED", "单应性矩阵计算失败", "Homography calculation failed", "astra-spatial", ErrorSeverity::Error, true, true},
        {3206, "HOMOGRAPHY_NOT_INVERTIBLE", "单应性矩阵不可逆", "Homography matrix is not invertible", "astra-spatial", ErrorSeverity::Error, true, true},
        {3207, "REPROJECTION_ERROR_EXCEEDED", "重投影误差超限", "Reprojection error exceeds the configured limit", "astra-spatial", ErrorSeverity::Warning, true, true},
        {3208, "CALIBRATION_CONFIGURATION_INVALID", "标定配置无效", "Calibration configuration is invalid", "astra-spatial", ErrorSeverity::Error, false, true},
        {3301, "SPATIAL_SCENE_MISSING", "空间场景不存在", "Spatial scene does not exist", "astra-spatial", ErrorSeverity::Warning, true, true},
        {3302, "SCENE_OBJECT_MISSING", "空间对象不存在", "Scene object does not exist", "astra-spatial", ErrorSeverity::Warning, true, true},
        {3303, "SPATIAL_ANCHOR_NOT_FOUND", "空间锚点不存在", "Spatial anchor does not exist", "astra-spatial", ErrorSeverity::Warning, true, true},
        {3304, "PROJECTION_TARGET_MISSING", "投影目标不存在", "Projection target does not exist", "astra-spatial", ErrorSeverity::Warning, true, true},
        {3305, "PROJECTION_TARGET_LOST", "投影目标已丢失", "Projection target was lost", "astra-spatial", ErrorSeverity::Error, true, true},
        {3306, "SCENE_STATE_INVALID", "场景状态无效", "Scene state is invalid", "astra-spatial", ErrorSeverity::Error, false, true},
        {3401, "OBSERVER_TRACKING_UNAVAILABLE", "观察者追踪不可用", "Observer tracking is unavailable", "astra-spatial", ErrorSeverity::Warning, true, true},
        {3402, "OBSERVER_DATA_INVALID", "观察者数据无效", "Observer data is invalid", "astra-spatial", ErrorSeverity::Warning, true, true},
        {3403, "COORDINATE_SYSTEM_UNSUPPORTED", "坐标系不支持", "Coordinate system is unsupported", "astra-spatial", ErrorSeverity::Error, false, true},
        {3404, "COORDINATE_CONVERSION_FAILED", "坐标转换失败", "Coordinate conversion failed", "astra-spatial", ErrorSeverity::Error, true, true},
        {3501, "SPATIAL_PROCESSING_TIMEOUT", "空间处理超时", "Spatial processing timed out", "astra-spatial", ErrorSeverity::Error, true, true},
        {3502, "OPENCV_PROCESSING_FAILED", "OpenCV处理失败", "OpenCV processing failed", "astra-spatial", ErrorSeverity::Error, true, true},
        {3503, "SPATIAL_CONFIGURATION_INVALID", "空间配置无效", "Spatial configuration is invalid", "astra-spatial", ErrorSeverity::Error, false, true},
        {3504, "SPATIAL_SERVICE_INTERNAL_ERROR", "空间服务内部错误", "Spatial service internal error", "astra-spatial", ErrorSeverity::Critical, true, true},
        {4001, "PROJECTION_DEVICE_UNAVAILABLE", "投影设备模拟器不可用", "Projection device simulator is unavailable", "astra-projection", ErrorSeverity::Error, true, true},
        {4002, "PROJECTION_SESSION_EXISTS", "投影会话已存在", "Projection session already exists", "astra-projection", ErrorSeverity::Warning, false, true},
        {4003, "PROJECTION_SESSION_MISSING", "投影会话不存在", "Projection session does not exist", "astra-projection", ErrorSeverity::Warning, false, true},
        {4004, "INVALID_PROJECTION_TRANSITION", "非法投影状态转换", "Invalid projection state transition", "astra-projection", ErrorSeverity::Error, false, true},
        {4005, "SCENE_INITIALIZATION_FAILED", "三维场景初始化失败", "3D scene initialization failed", "astra-display", ErrorSeverity::Error, true, true},
        {4006, "PROJECTION_OUTPUT_INITIALIZATION_FAILED", "投影输出初始化失败", "Projection output initialization failed", "astra-projection", ErrorSeverity::Critical, false, true},
        {4007, "PROJECTION_OUTPUT_DISCONNECTED", "投影输出已断开", "Projection output disconnected", "astra-projection", ErrorSeverity::Critical, true, true},
        {4008, "RENDER_GRAPH_DEPENDENCY_MISSING", "渲染图依赖缺失", "Render graph dependency is missing", "astra-projection", ErrorSeverity::Error, false, true},
        {4009, "RENDER_GRAPH_CYCLE_DETECTED", "渲染图存在循环依赖", "Render graph cycle detected", "astra-projection", ErrorSeverity::Error, false, true},
        {4010, "PROJECTION_RENDER_PASS_FAILED", "投影渲染通道失败", "Projection render pass failed", "astra-projection", ErrorSeverity::Critical, true, true},
        {4011, "PROJECTION_HOMOGRAPHY_INVALID", "投影单应矩阵无效", "Projection homography is invalid", "astra-projection", ErrorSeverity::Error, false, true},
        {4012, "PROJECTION_HOMOGRAPHY_NON_INVERTIBLE", "投影单应矩阵不可逆", "Projection homography is non-invertible", "astra-projection", ErrorSeverity::Error, false, true},
        {4013, "PROJECTION_HOMOGRAPHY_OUT_OF_BOUNDS", "投影单应映射越界", "Projection homography maps outside safe bounds", "astra-projection", ErrorSeverity::Error, false, true},
        {4014, "PROJECTION_MESH_WARP_INVALID", "投影网格形变无效", "Projection mesh warp is invalid", "astra-projection", ErrorSeverity::Error, false, true},
        {4015, "PROJECTION_PRIVACY_MASK_INVALID", "投影隐私遮罩无效", "Projection privacy mask is invalid", "astra-projection", ErrorSeverity::Critical, false, true},
        {4016, "PROJECTION_SAFE_CLEAR_FAILED", "投影安全清屏失败", "Projection safe clear failed", "astra-projection", ErrorSeverity::Critical, false, true},
        {4017, "PROJECTION_RENDER_TARGET_LOST", "投影渲染目标丢失", "Projection render target lost", "astra-projection", ErrorSeverity::Critical, true, true},
        {4018, "PROJECTION_OUTPUT_PRESENT_FAILED", "投影输出呈现失败", "Projection output present failed", "astra-projection", ErrorSeverity::Critical, true, true},
        {4301, "PROJECTION_DENIED_BY_PRIVACY_POLICY", "隐私策略拒绝投影", "Projection denied by privacy policy", "astra-policy", ErrorSeverity::Warning, false, true},
        {4302, "AUTHORIZED_PERSON_VERIFICATION_FAILED", "授权人员验证失败", "Authorized person verification failed", "astra-policy", ErrorSeverity::Warning, true, true},
        {5001, "PERMISSION_DENIED", "权限不足", "Permission denied", "astra-security", ErrorSeverity::Error, false, true},
        {5101, "SPATIAL_UI_SERVICE_NOT_STARTED", "空间界面服务未启动", "Spatial UI service is not started", "astra-spatial-ui", ErrorSeverity::Error, true, true},
        {5102, "SPATIAL_UI_INITIALIZATION_FAILED", "空间界面初始化失败", "Spatial UI initialization failed", "astra-spatial-ui", ErrorSeverity::Critical, true, true},
        {5103, "SPATIAL_UI_CONFIGURATION_INVALID", "空间界面配置无效", "Spatial UI configuration is invalid", "astra-spatial-ui", ErrorSeverity::Error, false, true},
        {5104, "SPATIAL_UI_COMPONENT_TYPE_UNSUPPORTED", "空间组件类型不支持", "Spatial UI component type is unsupported", "astra-spatial-ui", ErrorSeverity::Error, false, true},
        {5105, "SPATIAL_UI_COMPONENT_NOT_FOUND", "空间组件不存在", "Spatial UI component does not exist", "astra-spatial-ui", ErrorSeverity::Error, false, true},
        {5106, "SPATIAL_UI_COMPONENT_LIMIT_EXCEEDED", "空间组件超过数量限制", "Spatial UI component limit exceeded", "astra-spatial-ui", ErrorSeverity::Error, false, true},
        {5107, "SPATIAL_UI_COMPONENT_TRANSITION_INVALID", "空间组件生命周期转换非法", "Spatial UI component lifecycle transition is invalid", "astra-spatial-ui", ErrorSeverity::Error, false, true},
        {5201, "SPATIAL_WINDOW_NOT_FOUND", "空间窗口不存在", "Spatial window does not exist", "astra-spatial-ui", ErrorSeverity::Error, false, true},
        {5202, "SPATIAL_WINDOW_LIMIT_EXCEEDED", "空间窗口超过数量限制", "Spatial window limit exceeded", "astra-spatial-ui", ErrorSeverity::Error, false, true},
        {5203, "SPATIAL_WINDOW_TRANSITION_INVALID", "空间窗口状态转换非法", "Spatial window state transition is invalid", "astra-spatial-ui", ErrorSeverity::Error, false, true},
        {5204, "SPATIAL_WINDOW_OUT_OF_BOUNDS", "空间窗口超出安全区域", "Spatial window is outside safe bounds", "astra-spatial-ui", ErrorSeverity::Error, false, true},
        {5205, "SPATIAL_WINDOW_SIZE_INVALID", "空间窗口尺寸无效", "Spatial window size is invalid", "astra-spatial-ui", ErrorSeverity::Error, false, true},
        {5206, "SPATIAL_WINDOW_TARGET_UNAVAILABLE", "空间窗口目标不可用", "Spatial window display target is unavailable", "astra-spatial-ui", ErrorSeverity::Error, true, true},
        {5301, "SPATIAL_LAYOUT_TYPE_UNSUPPORTED", "空间布局类型不支持", "Spatial layout type is unsupported", "astra-spatial-ui", ErrorSeverity::Error, false, true},
        {5302, "SPATIAL_LAYOUT_CONSTRAINT_CONFLICT", "空间布局约束冲突", "Spatial layout constraints conflict", "astra-spatial-ui", ErrorSeverity::Error, false, true},
        {5303, "SPATIAL_LAYOUT_CALCULATION_FAILED", "空间布局计算失败", "Spatial layout calculation failed", "astra-spatial-ui", ErrorSeverity::Error, true, true},
        {5304, "SPATIAL_LAYOUT_AREA_INSUFFICIENT", "空间布局区域不足", "Spatial layout area is insufficient", "astra-spatial-ui", ErrorSeverity::Error, false, true},
        {5305, "SPATIAL_LAYOUT_RESTORE_FAILED", "空间布局恢复失败", "Spatial layout state restore failed", "astra-spatial-ui", ErrorSeverity::Error, true, true},
        {5401, "SPATIAL_INPUT_SOURCE_UNSUPPORTED", "空间输入源不支持", "Spatial input source is unsupported", "astra-spatial-ui", ErrorSeverity::Error, false, true},
        {5402, "SPATIAL_INPUT_EVENT_INVALID", "空间输入事件无效", "Spatial input event is invalid", "astra-spatial-ui", ErrorSeverity::Error, false, true},
        {5403, "SPATIAL_INPUT_TARGET_NOT_FOUND", "空间输入目标不存在", "Spatial input target does not exist", "astra-spatial-ui", ErrorSeverity::Error, false, true},
        {5404, "SPATIAL_INPUT_HIT_TEST_FAILED", "空间输入命中测试失败", "Spatial input hit test failed", "astra-spatial-ui", ErrorSeverity::Error, false, true},
        {5405, "SPATIAL_INPUT_ROUTING_FAILED", "空间输入路由失败", "Spatial input routing failed", "astra-spatial-ui", ErrorSeverity::Error, true, true},
        {5406, "SPATIAL_INPUT_BLOCKED_BY_SAFETY_LAYER", "空间输入被安全层阻止", "Spatial input was blocked by the safety layer", "astra-spatial-ui", ErrorSeverity::Warning, false, true},
        {5501, "SPATIAL_FOCUS_TARGET_NOT_FOUND", "空间焦点目标不存在", "Spatial focus target does not exist", "astra-spatial-ui", ErrorSeverity::Error, false, true},
        {5502, "SPATIAL_FOCUS_SCOPE_INVALID", "空间焦点范围无效", "Spatial focus scope is invalid", "astra-spatial-ui", ErrorSeverity::Error, false, true},
        {5503, "SPATIAL_FOCUS_PREEMPTION_DENIED", "空间焦点抢占被拒绝", "Spatial focus preemption was denied", "astra-spatial-ui", ErrorSeverity::Warning, false, true},
        {5504, "SPATIAL_FOCUS_RESTORE_FAILED", "空间焦点恢复失败", "Spatial focus restore failed", "astra-spatial-ui", ErrorSeverity::Error, true, true},
        {5601, "SPATIAL_INTERACTION_TRANSITION_INVALID", "空间交互状态转换非法", "Spatial interaction state transition is invalid", "astra-spatial-ui", ErrorSeverity::Error, false, true},
        {5602, "SPATIAL_DRAG_INVALID", "空间拖拽操作无效", "Spatial drag operation is invalid", "astra-spatial-ui", ErrorSeverity::Error, false, true},
        {5603, "SPATIAL_RESIZE_INVALID", "空间缩放操作无效", "Spatial resize operation is invalid", "astra-spatial-ui", ErrorSeverity::Error, false, true},
        {5604, "SPATIAL_ROTATION_INVALID", "空间旋转操作无效", "Spatial rotation operation is invalid", "astra-spatial-ui", ErrorSeverity::Error, false, true},
        {5605, "SPATIAL_INTERACTION_CANCELLED", "空间交互操作已取消", "Spatial interaction operation was cancelled", "astra-spatial-ui", ErrorSeverity::Warning, false, true},
        {5701, "SPATIAL_UI_PRIVACY_DISPLAY_DENIED", "隐私策略拒绝空间组件显示", "Privacy policy denied spatial component display", "astra-policy", ErrorSeverity::Warning, false, true},
        {5702, "SPATIAL_UI_PRIVACY_LEVEL_INVALID", "空间组件隐私等级无效", "Spatial component privacy level is invalid", "astra-policy", ErrorSeverity::Error, false, true},
        {5703, "SPATIAL_UI_DISPLAY_TARGET_DENIED", "空间组件显示目标不允许", "Spatial component display target is not allowed", "astra-policy", ErrorSeverity::Warning, false, true},
        {5704, "SPATIAL_UI_PRIVACY_UPDATE_FAILED", "空间界面隐私状态更新失败", "Spatial UI privacy state update failed", "astra-policy", ErrorSeverity::Critical, true, true},
        {5801, "SPATIAL_UI_STATE_SAVE_FAILED", "空间界面状态保存失败", "Spatial UI state save failed", "astra-spatial-ui", ErrorSeverity::Error, true, true},
        {5802, "SPATIAL_UI_STATE_LOAD_FAILED", "空间界面状态加载失败", "Spatial UI state load failed", "astra-spatial-ui", ErrorSeverity::Error, true, true},
        {5803, "SPATIAL_UI_STATE_SCHEMA_INVALID", "空间界面状态结构无效", "Spatial UI state schema is invalid", "astra-spatial-ui", ErrorSeverity::Error, false, true},
        {5804, "SPATIAL_UI_STATE_VERSION_INCOMPATIBLE", "空间界面状态版本不兼容", "Spatial UI state version is incompatible", "astra-spatial-ui", ErrorSeverity::Error, false, true},
        {5901, "SPATIAL_UI_ACCESSIBILITY_SEMANTICS_MISSING", "空间界面无障碍语义缺失", "Spatial UI accessibility semantics are missing", "astra-spatial-ui", ErrorSeverity::Error, false, true},
        {5902, "SPATIAL_UI_ACCESSIBILITY_ACTION_UNSUPPORTED", "空间界面无障碍动作不支持", "Spatial UI accessibility action is unsupported", "astra-spatial-ui", ErrorSeverity::Error, false, true},
        {5903, "SPATIAL_UI_ANIMATION_FAILED", "空间界面动画失败", "Spatial UI animation failed", "astra-spatial-ui", ErrorSeverity::Error, true, true},
        {5904, "SPATIAL_UI_RENDER_SUBMISSION_FAILED", "空间界面渲染提交失败", "Spatial UI render submission failed", "astra-spatial-ui", ErrorSeverity::Critical, true, true},
        {5905, "SPATIAL_UI_INTERNAL_ERROR", "空间界面内部错误", "Spatial UI internal error", "astra-spatial-ui", ErrorSeverity::Critical, false, true},
        {9001, "INTERNAL_UNCLASSIFIED", "未分类内部错误", "Unclassified internal error", "astra-common", ErrorSeverity::Critical, false, true},
    };
    return values;
}

QString severityName(ErrorSeverity severity)
{
    switch (severity) {
    case ErrorSeverity::Warning: return QStringLiteral("WARNING");
    case ErrorSeverity::Error: return QStringLiteral("ERROR");
    case ErrorSeverity::Critical: return QStringLiteral("CRITICAL");
    }
    return QStringLiteral("CRITICAL");
}

} // namespace

QVector<ErrorDescriptor> ErrorCodeRegistry::all() { return descriptors(); }

ErrorDescriptor ErrorCodeRegistry::lookup(int code)
{
    for (const auto &descriptor : descriptors()) {
        if (descriptor.code == code) return descriptor;
    }
    for (const auto &descriptor : descriptors()) {
        if (descriptor.code == 9001) return descriptor;
    }
    return {};
}

QJsonDocument ErrorCodeRegistry::exportJson()
{
    QJsonArray values;
    for (const auto &descriptor : descriptors()) {
        values.append(QJsonObject {{"code", descriptor.code}, {"symbol", descriptor.symbol},
                                   {"message_zh", descriptor.messageZh}, {"message_en", descriptor.messageEn},
                                   {"module", descriptor.module}, {"severity", severityName(descriptor.severity)},
                                   {"retryable", descriptor.retryable}, {"audit_required", descriptor.auditRequired}});
    }
    return QJsonDocument(QJsonObject {{"version", 1}, {"codes", values}});
}

} // namespace astra::common
