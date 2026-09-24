#include "controllers/ShellController.h"

#include "astra/common/ErrorCode.h"
#include "astra/common/Identifiers.h"
#include "astra/common/PrivacyLevel.h"

#include <QFile>

#include <QUrl>

namespace astra::shell {
namespace {

QString readTokenFile(const char *environmentName)
{
    QFile file {qEnvironmentVariable(environmentName)};
    if (!file.open(QIODevice::ReadOnly)) return {};
    return QString::fromUtf8(file.readAll()).trimmed();
}

QString registeredErrorMessage(astra::common::ErrorCode code)
{
    return astra::common::errorMessage(code);
}

} // namespace

ShellController::ShellController(const SimulatorConfig &config, QString auditPath, QObject *parent)
    : ShellController(config,
                      std::move(auditPath),
                      IntentServiceClient(qEnvironmentVariable("ASTRA_INTENT_SOCKET_PATH"),
                                          qEnvironmentVariable("ASTRA_INTENT_CAPABILITY_TOKEN"),
                                          1000),
                      qEnvironmentVariableIntValue("ASTRA_P2_MODE") == 1,
                      parent)
{
}

ShellController::ShellController(const SimulatorConfig &config,
                                 QString auditPath,
                                 IntentServiceClient intentClient,
                                 bool p2Mode,
                                 QObject *parent)
    : QObject(parent)
    , intentClient_(std::move(intentClient))
    , spatialClient_(qEnvironmentVariable("ASTRA_SPATIAL_SOCKET_PATH"), qEnvironmentVariable("ASTRA_SPATIAL_CAPABILITY_TOKEN"), 1000)
    , taskModel_(config.recentTaskLimit)
    , projectionServiceClient_(qEnvironmentVariable("ASTRA_P4_PROJECTION_SOCKET"),
                               qEnvironmentVariableIsSet("ASTRA_P4_PROJECTION_TOKEN_FILE")
                                   ? readTokenFile("ASTRA_P4_PROJECTION_TOKEN_FILE") : qEnvironmentVariable("ASTRA_P4_PROJECTION_CAPABILITY"))
    , spatialUIServiceClient_(qEnvironmentVariable("ASTRA_P5_SPATIAL_UI_SOCKET"), readTokenFile("ASTRA_P5_SPATIAL_UI_TOKEN_FILE"))
    , audit_(std::move(auditPath))
    , simulatedAuthorized_(config.simulatedAuthorized)
    , p2Mode_(p2Mode)
{
    systemState_.setConfigurationStatus(QStringLiteral("VALID"));
    if (spatialUIServiceClient_.isConfigured()) {
        refreshSpatialUI();
        spatialUIPollTimer_.setInterval(100);
        connect(&spatialUIPollTimer_, &QTimer::timeout, this, &ShellController::refreshSpatialUI);
        spatialUIPollTimer_.start();
    }
    if (qEnvironmentVariableIntValue("ASTRA_P3_MODE") == 1) {
        spatialStatePollTimer_.setInterval(250);
        connect(&spatialStatePollTimer_, &QTimer::timeout, this, &ShellController::refreshSpatial);
        spatialStatePollTimer_.start();
    }
}

QString ShellController::projectionState() const { return activeProjectionState(); }
QString ShellController::taskResult() const { return taskResult_; }
QString ShellController::errorMessage() const { return errorMessage_; }
int ShellController::lastErrorCode() const { return lastErrorCode_; }
QString ShellController::currentRawText() const { return currentRawText_; }
QString ShellController::currentIntent() const { return currentIntent_; }
QString ShellController::currentTargetSpace() const { return currentTargetSpace_; }
QString ShellController::currentPrivacyLevel() const { return currentPrivacyLevel_; }
QString ShellController::currentTaskStatus() const { return currentTaskStatus_; }
QString ShellController::currentRequestId() const { return currentRequestId_; }
bool ShellController::projectionFullscreen() const { return projectionFullscreen_; }
bool ShellController::p4ProjectionActive() const { return p4ProjectionActive_; }
QString ShellController::p4FramePath() const
{
    return p4ProjectionActive_ ? QUrl::fromLocalFile(projectionServiceClient_.framePath()).toString() : QString {};
}
SystemStateModel *ShellController::systemState() { return &systemState_; }
TaskModel *ShellController::recentTasks() { return &taskModel_; }
ProjectionStateModel *ShellController::projectionModel() { return &projectionModel_; }
SpatialUIStateModel *ShellController::spatialUIModel() { return &spatialUIModel_; }
bool ShellController::spatialUIAvailable() const { return spatialUIModel_.status() == QStringLiteral("READY"); }

void ShellController::refreshSpatialUI()
{
    const auto result = spatialUIServiceClient_.status();
    if (result.ok) {
        const auto components = spatialUIServiceClient_.components();
        if (components.ok) {
            spatialUIModel_.applyStatus(result.result);
            spatialUIModel_.applyComponents(components.result);
            if (result.result.value(QStringLiteral("projection_frame_id")).toInteger() > 0) {
                p4ProjectionActive_ = projectionServiceClient_.attachToCurrentOutput().ok;
            } else {
                projectionServiceClient_.detachOutput();
                p4ProjectionActive_ = false;
            }
        } else {
            spatialUIModel_.setOffline();
            projectionServiceClient_.detachOutput();
            p4ProjectionActive_ = false;
        }
    } else {
        spatialUIModel_.setOffline();
        projectionServiceClient_.detachOutput();
        p4ProjectionActive_ = false;
    }
    emit spatialUIChanged();
}

void ShellController::createSpatialTaskCard(const QString &title, const QString &summary, const QString &privacyLevel)
{
    const auto result = spatialUIServiceClient_.createTaskCard(title, summary, privacyLevel);
    if (!result.ok) {
        setError(result.errorCode, result.message);
        spatialUIModel_.setOffline();
    } else {
        refreshSpatialUI();
    }
    emit spatialUIChanged();
    emit taskChanged();
}

void ShellController::activateSpatialComponent(const QString &componentId, const QString &action)
{
    const auto result = spatialUIServiceClient_.activateComponent(componentId, action);
    if (!result.ok) {
        setError(result.errorCode, result.message);
    } else {
        clearError();
        refreshSpatialUI();
    }
    emit spatialUIChanged();
    emit taskChanged();
}

void ShellController::routeSpatialInput(const QString &componentId,
                                        const QString &eventType,
                                        const QString &sourceType,
                                        const QString &coordinateSystem,
                                        double x,
                                        double y)
{
    const auto result = spatialUIServiceClient_.routeComponentInput(
        componentId, eventType, sourceType, coordinateSystem, x, y);
    if (!result.ok) {
        setError(result.errorCode, result.message);
    } else {
        clearError();
        refreshSpatialUI();
    }
    emit spatialUIChanged();
    emit taskChanged();
}
IntentResultModel *ShellController::intentResult() { return &intentResultModel_; }
SpatialStateModel *ShellController::spatialState() { return &spatialState_; }

void ShellController::setConfigurationStatus(const QString &status)
{
    systemState_.setConfigurationStatus(status);
    emit stateChanged();
}

void ShellController::setConfigurationDiagnostics(const QStringList &warnings, bool usingFallback)
{
    systemState_.setConfigurationDiagnostics(warnings, usingFallback);
    emit stateChanged();
}

void ShellController::applySpatialState(const QJsonObject &state)
{
    const auto unsafeSpatialState = [](const QString &value) {
        return value == QStringLiteral("DEGRADED") || value == QStringLiteral("LOST") || value == QStringLiteral("ERROR");
    };
    const auto unsafeTargetState = [](const QString &value) {
        return value == QStringLiteral("DEGRADED") || value == QStringLiteral("LOST") || value == QStringLiteral("INVALID");
    };
    const bool previouslyUnsafe = unsafeSpatialState(spatialState_.state()) || unsafeTargetState(spatialState_.targetState());
    spatialState_.apply(state);
    systemState_.setSpatialStatus(spatialState_.state());
    const QJsonObject offset = state.value(QStringLiteral("observer_camera_offset")).toObject();
    if (!offset.isEmpty()) {
        projectionModel_.setSpatialCameraOffset(offset.value(QStringLiteral("x")).toDouble(), offset.value(QStringLiteral("y")).toDouble(),
                                                offset.value(QStringLiteral("z")).toDouble());
    }
    const bool unsafe = unsafeSpatialState(spatialState_.state()) || unsafeTargetState(spatialState_.targetState());
    if (unsafe && !previouslyUnsafe) {
        SimulatedIntent safetyIntent = lifecycleIntent(QStringLiteral("spatial_safe_pause"));
        safetyIntent.privacyLevel = activePrivacy_.isEmpty() ? QStringLiteral("PUBLIC") : activePrivacy_;
        safetyIntent.targetSpace = activeTarget_.isEmpty() ? QStringLiteral("desk") : activeTarget_;
        const int errorCode = spatialState_.lastErrorCode() == 0 ? static_cast<int>(astra::common::ErrorCode::ProjectionTargetLost)
                                                                  : spatialState_.lastErrorCode();
        if (hasActiveSession()) projectionService_.stop();
        projectionModel_.clear();
        systemState_.setProjectionStatus(QStringLiteral("IDLE"));
        activePrivacy_.clear();
        activeTarget_.clear();
        activeSessionId_.clear();
        setError(errorCode,
                 spatialState_.warning().isEmpty()
                     ? registeredErrorMessage(static_cast<astra::common::ErrorCode>(errorCode))
                     : spatialState_.warning());
        writeAudit(QStringLiteral("spatial_safe_pause"),
                   safetyIntent,
                   QStringLiteral("success"),
                   errorCode,
                   {{QStringLiteral("action"), QStringLiteral("clear_sensitive_projection_content")},
                    {QStringLiteral("spatial_state"), spatialState_.state()},
                    {QStringLiteral("target_state"), spatialState_.targetState()}});
        addTask(safetyIntent, QStringLiteral("SAFE_PAUSED"));
    }
    emit taskChanged();
    emit stateChanged();
}

void ShellController::invokeSpatial(const QString &method, const QJsonObject &params)
{
    const SpatialServiceResult result = spatialClient_.call(method, params);
    if (!result.transportOk) {
        applySpatialState({{QStringLiteral("state"), QStringLiteral("ERROR")},
                           {QStringLiteral("input_source"), spatialState_.inputSource()},
                           {QStringLiteral("quality"), QStringLiteral("UNUSABLE")},
                           {QStringLiteral("warning"), result.errorMessage},
                           {QStringLiteral("last_error_code"), result.errorCode},
                           {QStringLiteral("target"), QJsonObject {}}});
        return;
    }
    if (result.result.contains(QStringLiteral("input_source"))) {
        applySpatialState(result.result);
    } else if (method != QStringLiteral("spatial.state")) {
        refreshSpatial();
    }
}

void ShellController::startSpatial()
{
    invokeSpatial(QStringLiteral("spatial.source.start"), {{QStringLiteral("source"), QStringLiteral("SIMULATION")}});
}

void ShellController::refreshSpatial() { invokeSpatial(QStringLiteral("spatial.state")); }
void ShellController::detectSpatial() { invokeSpatial(QStringLiteral("spatial.detect")); }
void ShellController::selectSpatial() { invokeSpatial(QStringLiteral("spatial.select")); }
void ShellController::calibrateSpatial()
{
    invokeSpatial(QStringLiteral("spatial.calibrate"), {{QStringLiteral("corners"), spatialState_.calibrationCorners()}});
}
void ShellController::resetSpatial() { invokeSpatial(QStringLiteral("spatial.reset")); }

SimulatedIntent ShellController::lifecycleIntent(const QString &intent) const
{
    return {QString::fromStdString(astra::common::newUuid()), QString::fromStdString(astra::common::newUuid()), {}, intent, 1.0, QStringLiteral("desk"),
            QStringLiteral("PUBLIC"), {}, QString::fromStdString(astra::common::utcTimestamp())};
}

void ShellController::recordApplicationStarted()
{
    const auto intent = lifecycleIntent(QStringLiteral("application_lifecycle"));
    writeAudit(QStringLiteral("application_started"), intent, QStringLiteral("success"));
}

void ShellController::recordApplicationStopped()
{
    const auto intent = lifecycleIntent(QStringLiteral("application_lifecycle"));
    writeAudit(QStringLiteral("application_stopped"), intent, QStringLiteral("success"));
}

void ShellController::recordConfigurationResult(bool valid)
{
    const auto intent = lifecycleIntent(QStringLiteral("configuration"));
    writeAudit(valid ? QStringLiteral("configuration_loaded") : QStringLiteral("configuration_failed"),
               intent,
               valid ? QStringLiteral("success") : QStringLiteral("failed"),
               valid ? 0 : static_cast<int>(astra::common::ErrorCode::ConfigurationInvalid));
}

void ShellController::recordApplicationInitializationFailure()
{
    const auto intent = lifecycleIntent(QStringLiteral("application_lifecycle"));
    writeAudit(QStringLiteral("application_initialization_failed"),
               intent,
               QStringLiteral("failed"),
               static_cast<int>(astra::common::ErrorCode::ApplicationInitializationFailed));
}

void ShellController::startP4FixtureDemo()
{
    if (!projectionServiceClient_.isConfigured()) return;
    submit(QStringLiteral("开始投影"), QStringLiteral("PUBLIC"));
}

void ShellController::clearError()
{
    errorMessage_.clear();
    lastErrorCode_ = 0;
}

void ShellController::setError(int code, const QString &message)
{
    lastErrorCode_ = code;
    errorMessage_ = message;
}

void ShellController::writeAudit(const QString &event,
                                 const SimulatedIntent &intent,
                                 const QString &result,
                                 int errorCode,
                                 const QJsonObject &details)
{
    AuditEvent auditEvent;
    auditEvent.event = event;
    auditEvent.traceId = intent.traceId;
    auditEvent.requestId = intent.requestId;
    const QString runtimeSessionId = activeProjectionSessionId();
    auditEvent.sessionId = runtimeSessionId.isEmpty() ? activeSessionId_ : runtimeSessionId;
    auditEvent.module = QStringLiteral("shell.controller");
    auditEvent.actor = QStringLiteral("local-user");
    auditEvent.intent = intent.intent;
    auditEvent.privacyLevel = intent.privacyLevel;
    auditEvent.result = result;
    auditEvent.errorCode = errorCode;
    auditEvent.message = errorCode == 0 ? event : registeredErrorMessage(static_cast<astra::common::ErrorCode>(errorCode));
    auditEvent.details = details;
    if (!audit_.write(auditEvent)) systemState_.setAuditStatus(QStringLiteral("DEGRADED"));
}

void ShellController::addTask(const SimulatedIntent &intent, const QString &status)
{
    taskModel_.append({intent.rawText, intent.intent, intent.targetSpace, intent.privacyLevel, status,
                       errorMessage_, intent.requestId, intent.timestamp});
    currentTaskStatus_ = status;
}

bool ShellController::hasActiveSession() const
{
    const QString state = activeProjectionState();
    return state == QStringLiteral("ACTIVE") || state == QStringLiteral("PAUSED");
}

QString ShellController::activeProjectionState() const
{
    return p4ProjectionActive_ ? projectionServiceClient_.state() : projectionService_.state();
}

QString ShellController::activeProjectionSessionId() const
{
    return p4ProjectionActive_ ? projectionServiceClient_.sessionId() : projectionService_.sessionId();
}

ProjectionOperationResult ShellController::startProjectionRuntime()
{
    if (projectionServiceClient_.isConfigured()) {
        const auto p4Result = projectionServiceClient_.start();
        if (p4Result.ok) {
            p4ProjectionActive_ = true;
            return p4Result;
        }
        p4ProjectionActive_ = false;
    }
    return projectionService_.start();
}

ProjectionOperationResult ShellController::pauseProjectionRuntime()
{
    if (p4ProjectionActive_) return projectionServiceClient_.pause();
    return projectionService_.pause();
}

ProjectionOperationResult ShellController::resumeProjectionRuntime()
{
    if (p4ProjectionActive_) return projectionServiceClient_.resume();
    return projectionService_.resume();
}

ProjectionOperationResult ShellController::stopProjectionRuntime()
{
    if (!p4ProjectionActive_) return projectionService_.stop();
    if (spatialUIServiceClient_.isConfigured()) {
        const auto cleared = spatialUIServiceClient_.notifyTargetLost();
        projectionServiceClient_.detachOutput();
        p4ProjectionActive_ = false;
        return cleared.ok ? ProjectionOperationResult {true, 0, {}, {}, QStringLiteral("IDLE")}
                          : ProjectionOperationResult {false, cleared.errorCode, cleared.message, {}, QStringLiteral("ERROR")};
    }
    const auto p4Result = projectionServiceClient_.stop();
    p4ProjectionActive_ = false;
    if (p4Result.ok) return p4Result;
    return {true, 0, QStringLiteral("P4 service unavailable; P1 hide fallback applied"), {}, QStringLiteral("IDLE")};
}

void ShellController::applyProjectionResult(const SimulatedIntent &intent,
                                            const ProjectionOperationResult &result,
                                            const QString &successEvent,
                                            const QString &failureEvent)
{
    if (result.ok) {
        if (!result.sessionId.isEmpty()) activeSessionId_ = result.sessionId;
        activePrivacy_ = intent.privacyLevel;
        activeTarget_ = intent.targetSpace;
        projectionModel_.applySession(result.state, result.sessionId, activePrivacy_, activeTarget_);
        systemState_.setProjectionStatus(result.state);
        writeAudit(successEvent, intent, QStringLiteral("success"), 0,
                   {{QStringLiteral("target_space"), activeTarget_}});
        return;
    }

    setError(result.errorCode, result.message);
    if (result.errorCode == static_cast<int>(astra::common::ErrorCode::PrivacyPolicyDenied)
        || result.errorCode == static_cast<int>(astra::common::ErrorCode::AuthorizationFailed)) {
        projectionModel_.clear();
        systemState_.setProjectionStatus(QStringLiteral("IDLE"));
        activePrivacy_.clear();
        activeTarget_.clear();
    }
    writeAudit(failureEvent, intent, QStringLiteral("failed"), result.errorCode);
}

void ShellController::submit(const QString &text, const QString &privacy)
{
    if (!p2Mode_) {
        executeIntent(intentSimulator_.parse(text, privacy));
        return;
    }

    const IntentServiceResult result = intentClient_.analyze(text,
                                                             privacy,
                                                             projectionService_.state(),
                                                             QStringLiteral("development_workspace"),
                                                             QStringLiteral("demo-device"));
    if (!result.transportOk) {
        intentResultModel_.setOfflineFallback(result.errorMessage);
        SimulatedIntent fallback = intentSimulator_.parse(text, privacy);
        const bool safeFallback = fallback.intent == QStringLiteral("stop_projection")
            || fallback.intent == QStringLiteral("hide_projection_content")
            || fallback.intent == QStringLiteral("show_system_status");
        if (safeFallback) {
            executeIntent(fallback);
            return;
        }
        fallback.intent = QStringLiteral("unknown");
        fallback.confidence = 0.0;
        IntentServiceResult rejected = result;
        rejected.executionPolicy = QStringLiteral("REJECT");
        rejected.errorCode = static_cast<int>(astra::common::ErrorCode::IntentServiceInternal);
        rejected.errorMessage = registeredErrorMessage(astra::common::ErrorCode::IntentServiceInternal);
        presentNonExecutableIntent(fallback, rejected);
        return;
    }

    intentResultModel_.apply(result);
    const SimulatedIntent intent = serviceIntent(text, privacy, result);
    if (result.executionPolicy != QStringLiteral("AUTO_EXECUTE")) {
        presentNonExecutableIntent(intent, result);
        return;
    }
    executeIntent(intent);
}

SimulatedIntent ShellController::serviceIntent(const QString &text,
                                               const QString &selectedPrivacy,
                                               const IntentServiceResult &result) const
{
    QString mappedIntent = result.intent;
    if (mappedIntent == QStringLiteral("start_projection")) mappedIntent = QStringLiteral("project_3d_model");
    if (mappedIntent == QStringLiteral("pause_projection")) mappedIntent = QStringLiteral("pause_rotation");
    if (mappedIntent == QStringLiteral("resume_projection")) mappedIntent = QStringLiteral("resume_rotation");
    if (mappedIntent == QStringLiteral("reset_projection_view") || mappedIntent == QStringLiteral("reset_model")) {
        mappedIntent = QStringLiteral("reset_view");
    }
    if (mappedIntent == QStringLiteral("enter_projection_fullscreen")) mappedIntent = QStringLiteral("enter_fullscreen");
    if (mappedIntent == QStringLiteral("exit_projection_fullscreen")) mappedIntent = QStringLiteral("exit_fullscreen");
    const QString target = result.slotValues.value(QStringLiteral("target_space"), QStringLiteral("desk")).toString();
    const QString resultPrivacy = result.slotValues.value(QStringLiteral("privacy_level"), selectedPrivacy).toString();
    return {result.traceId,
            result.requestId,
            text,
            mappedIntent,
            result.confidence,
            target.isEmpty() ? QStringLiteral("desk") : target,
            resultPrivacy.isEmpty() ? selectedPrivacy : resultPrivacy,
            result.slotValues,
            QString::fromStdString(astra::common::utcTimestamp())};
}

void ShellController::presentNonExecutableIntent(const SimulatedIntent &intent, const IntentServiceResult &result)
{
    taskResult_ = intent.intent;
    currentRawText_ = intent.rawText;
    currentIntent_ = intent.intent;
    currentTargetSpace_ = intent.targetSpace;
    currentPrivacyLevel_ = intent.privacyLevel;
    currentRequestId_ = intent.requestId;
    clearError();
    QString status = QStringLiteral("REJECTED");
    QString event = QStringLiteral("intent_rejected");
    if (result.executionPolicy == QStringLiteral("REQUIRE_CONFIRMATION")) {
        status = QStringLiteral("AWAITING_CONFIRMATION");
        event = QStringLiteral("confirmation_required");
        pendingIntent_ = intent;
    } else if (result.executionPolicy == QStringLiteral("ASK_CLARIFICATION")) {
        status = QStringLiteral("NEEDS_CLARIFICATION");
        event = QStringLiteral("ambiguity_detected");
        pendingIntent_.reset();
    } else {
        pendingIntent_.reset();
    }
    if (result.errorCode != 0) setError(result.errorCode, result.errorMessage);
    writeAudit(event, intent, status.toLower(), result.errorCode);
    addTask(intent, status);
    emit taskChanged();
    emit stateChanged();
}

void ShellController::executeIntent(const SimulatedIntent &intent)
{
    taskResult_ = intent.intent;
    currentRawText_ = intent.rawText;
    currentIntent_ = intent.intent;
    currentTargetSpace_ = intent.targetSpace;
    currentPrivacyLevel_ = intent.privacyLevel;
    currentRequestId_ = intent.requestId;
    clearError();
    writeAudit(QStringLiteral("intent_received"), intent, QStringLiteral("received"));
    writeAudit(intent.intent == QStringLiteral("unknown") ? QStringLiteral("intent_unknown") : QStringLiteral("intent_parsed"),
               intent,
               intent.intent == QStringLiteral("unknown") ? QStringLiteral("rejected") : QStringLiteral("success"),
               intent.intent == QStringLiteral("unknown") ? static_cast<int>(astra::common::ErrorCode::IntentUnrecognized) : 0);

    const auto level = astra::common::privacyLevelFromString(intent.privacyLevel.toStdString());
    if (!level || intent.intent == QStringLiteral("unknown")) {
        setError(static_cast<int>(astra::common::ErrorCode::IntentUnrecognized),
                 registeredErrorMessage(astra::common::ErrorCode::IntentUnrecognized));
        addTask(intent, QStringLiteral("FAILED"));
    } else if (intent.intent == QStringLiteral("project_3d_model")) {
        writeAudit(QStringLiteral("projection_start_requested"), intent, QStringLiteral("requested"));
        const bool spatialUnsafe = spatialState_.state() == QStringLiteral("DEGRADED") || spatialState_.state() == QStringLiteral("LOST")
            || spatialState_.state() == QStringLiteral("ERROR") || spatialState_.targetState() == QStringLiteral("DEGRADED")
            || spatialState_.targetState() == QStringLiteral("LOST") || spatialState_.targetState() == QStringLiteral("INVALID");
        if (spatialUnsafe) {
            const int errorCode = spatialState_.lastErrorCode() == 0 ? static_cast<int>(astra::common::ErrorCode::ProjectionTargetLost)
                                                                      : spatialState_.lastErrorCode();
            setError(errorCode, registeredErrorMessage(static_cast<astra::common::ErrorCode>(errorCode)));
            projectionModel_.clear();
            systemState_.setProjectionStatus(QStringLiteral("IDLE"));
            writeAudit(QStringLiteral("spatial_safe_pause"), intent, QStringLiteral("rejected"), errorCode,
                       {{QStringLiteral("action"), QStringLiteral("block_projection_until_spatial_recovery")}});
            addTask(intent, QStringLiteral("SAFE_PAUSED"));
        } else {
        const ProjectionPolicyDecision policy = ProjectionPolicyService::evaluate(
            {*level, simulatedAuthorized_, ProjectionAction::Start, activeProjectionState()});
        if (policy.allowed) {
            applyProjectionResult(intent,
                                  startProjectionRuntime(),
                                  QStringLiteral("projection_started"),
                                  QStringLiteral("projection_error"));
        } else {
            applyProjectionResult(intent,
                                  {false, policy.errorCode, policy.reason, {}, QStringLiteral("DENIED")},
                                  QStringLiteral("projection_started"),
                                  QStringLiteral("projection_denied"));
            if (policy.clearProjectionContent) {
                projectionModel_.clear();
                systemState_.setProjectionStatus(QStringLiteral("IDLE"));
                activePrivacy_.clear();
                activeTarget_.clear();
            }
        }
        addTask(intent, lastErrorCode_ == 0 ? QStringLiteral("ACTIVE") : QStringLiteral("DENIED"));
        }
    } else if (intent.intent == QStringLiteral("stop_projection")) {
        applyProjectionResult(intent,
                              stopProjectionRuntime(),
                              QStringLiteral("projection_stopped"),
                              QStringLiteral("projection_error"));
        if (lastErrorCode_ == 0) {
            projectionModel_.clear();
            systemState_.setProjectionStatus(QStringLiteral("IDLE"));
            activePrivacy_.clear();
            activeTarget_.clear();
            activeSessionId_.clear();
        }
        addTask(intent, lastErrorCode_ == 0 ? QStringLiteral("IDLE") : QStringLiteral("FAILED"));
    } else if (intent.intent == QStringLiteral("hide_projection_content")) {
        if (hasActiveSession()) {
            applyProjectionResult(intent,
                                  projectionService_.stop(),
                                  QStringLiteral("projection_stopped"),
                                  QStringLiteral("projection_error"));
        }
        projectionModel_.clear();
        systemState_.setProjectionStatus(QStringLiteral("IDLE"));
        activePrivacy_.clear();
        activeTarget_.clear();
        activeSessionId_.clear();
        addTask(intent, QStringLiteral("IDLE"));
    } else if (intent.intent == QStringLiteral("pause_rotation")) {
        pauseRotation();
        addTask(intent, lastErrorCode_ == 0 ? QStringLiteral("PAUSED") : QStringLiteral("FAILED"));
    } else if (intent.intent == QStringLiteral("resume_rotation")) {
        resumeRotation();
        addTask(intent, lastErrorCode_ == 0 ? QStringLiteral("ACTIVE") : QStringLiteral("FAILED"));
    } else if (intent.intent == QStringLiteral("reset_view")) {
        resetView();
        addTask(intent, lastErrorCode_ == 0 ? activeProjectionState() : QStringLiteral("FAILED"));
    } else if (intent.intent == QStringLiteral("enter_fullscreen")) {
        enterFullscreen();
        addTask(intent, QStringLiteral("COMPLETED"));
    } else if (intent.intent == QStringLiteral("exit_fullscreen")) {
        exitFullscreen();
        addTask(intent, QStringLiteral("COMPLETED"));
    } else if (intent.intent == QStringLiteral("rotate_model")) {
        if (hasActiveSession()) {
            projectionModel_.setView(projectionModel_.sceneRotation() + 15.0, projectionModel_.zoom());
            writeAudit(QStringLiteral("view_rotated"), intent, QStringLiteral("success"));
            addTask(intent, activeProjectionState());
        } else {
            setError(static_cast<int>(astra::common::ErrorCode::ProjectionSessionMissing),
                     registeredErrorMessage(astra::common::ErrorCode::ProjectionSessionMissing));
            addTask(intent, QStringLiteral("FAILED"));
        }
    } else if (intent.intent == QStringLiteral("zoom_in_model") || intent.intent == QStringLiteral("zoom_out_model")) {
        const double delta = intent.intent == QStringLiteral("zoom_in_model") ? 0.15 : -0.15;
        changeZoom(delta);
        addTask(intent, lastErrorCode_ == 0 ? projectionService_.state() : QStringLiteral("FAILED"));
    } else {
        writeAudit(QStringLiteral("system_status_shown"), intent, QStringLiteral("success"));
        addTask(intent, QStringLiteral("COMPLETED"));
    }

    emit taskChanged();
    emit stateChanged();
}

void ShellController::confirmIntent()
{
    if (!pendingIntent_.has_value() || intentResultModel_.confirmationId().isEmpty()) {
        setError(static_cast<int>(astra::common::ErrorCode::ConfirmationExpired),
                 registeredErrorMessage(astra::common::ErrorCode::ConfirmationExpired));
        emit taskChanged();
        return;
    }
    const ConfirmationOperationResult result = intentClient_.confirm(intentResultModel_.confirmationId());
    if (!result.transportOk || result.status != QStringLiteral("ACCEPTED")) {
        const int code = result.errorCode == 0 ? static_cast<int>(astra::common::ErrorCode::ConfirmationExpired) : result.errorCode;
        setError(code,
                 result.errorMessage.isEmpty()
                     ? registeredErrorMessage(static_cast<astra::common::ErrorCode>(code))
                     : result.errorMessage);
        emit taskChanged();
        return;
    }
    const SimulatedIntent intent = *pendingIntent_;
    pendingIntent_.reset();
    intentResultModel_.clearConfirmation();
    executeIntent(intent);
}

void ShellController::rejectIntent()
{
    if (!pendingIntent_.has_value() || intentResultModel_.confirmationId().isEmpty()) return;
    const SimulatedIntent intent = *pendingIntent_;
    const ConfirmationOperationResult result = intentClient_.reject(intentResultModel_.confirmationId());
    pendingIntent_.reset();
    intentResultModel_.clearConfirmation();
    setError(static_cast<int>(astra::common::ErrorCode::ConfirmationRejected),
             result.errorMessage.isEmpty()
                 ? registeredErrorMessage(astra::common::ErrorCode::ConfirmationRejected)
                 : result.errorMessage);
    addTask(intent, QStringLiteral("REJECTED"));
    writeAudit(QStringLiteral("confirmation_rejected"), intent, QStringLiteral("rejected"), lastErrorCode_);
    emit taskChanged();
    emit stateChanged();
}

void ShellController::stopProjection()
{
    submit(QStringLiteral("停止投影"), activePrivacy_.isEmpty() ? QStringLiteral("PUBLIC") : activePrivacy_);
}

void ShellController::pauseRotation()
{
    clearError();
    const SimulatedIntent intent = intentSimulator_.parse(QStringLiteral("暂停旋转"), activePrivacy_.isEmpty() ? QStringLiteral("PUBLIC") : activePrivacy_);
    const auto result = pauseProjectionRuntime();
    applyProjectionResult(intent, result, QStringLiteral("projection_paused"), QStringLiteral("projection_error"));
    if (result.ok) projectionModel_.setRotationPaused(true);
    emit taskChanged();
    emit stateChanged();
}

void ShellController::resumeRotation()
{
    clearError();
    const SimulatedIntent intent = intentSimulator_.parse(QStringLiteral("继续旋转"), activePrivacy_.isEmpty() ? QStringLiteral("PUBLIC") : activePrivacy_);
    const auto result = resumeProjectionRuntime();
    applyProjectionResult(intent, result, QStringLiteral("projection_resumed"), QStringLiteral("projection_error"));
    if (result.ok) projectionModel_.setRotationPaused(false);
    emit taskChanged();
    emit stateChanged();
}

void ShellController::resetView()
{
    clearError();
    const SimulatedIntent intent = intentSimulator_.parse(QStringLiteral("重置视角"), activePrivacy_.isEmpty() ? QStringLiteral("PUBLIC") : activePrivacy_);
    if (!hasActiveSession()) {
        setError(static_cast<int>(astra::common::ErrorCode::ProjectionSessionMissing),
                 registeredErrorMessage(astra::common::ErrorCode::ProjectionSessionMissing));
        writeAudit(QStringLiteral("projection_error"), intent, QStringLiteral("failed"), lastErrorCode_);
    } else {
        projectionModel_.setView(0.0, 1.0);
        writeAudit(QStringLiteral("view_reset"), intent, QStringLiteral("success"));
    }
    emit taskChanged();
}

void ShellController::changeZoom(double delta)
{
    clearError();
    const SimulatedIntent intent = intentSimulator_.parse(QStringLiteral("旋转模型"), activePrivacy_.isEmpty() ? QStringLiteral("PUBLIC") : activePrivacy_);
    if (!hasActiveSession()) {
        setError(static_cast<int>(astra::common::ErrorCode::ProjectionSessionMissing),
                 registeredErrorMessage(astra::common::ErrorCode::ProjectionSessionMissing));
        writeAudit(QStringLiteral("projection_error"), intent, QStringLiteral("failed"), lastErrorCode_);
    } else {
        projectionModel_.setView(projectionModel_.sceneRotation(), projectionModel_.zoom() + delta);
        writeAudit(QStringLiteral("view_zoom_changed"), intent, QStringLiteral("success"));
    }
    emit taskChanged();
}

void ShellController::rotateView(double delta)
{
    clearError();
    const SimulatedIntent intent = intentSimulator_.parse(QStringLiteral("旋转模型"), activePrivacy_.isEmpty() ? QStringLiteral("PUBLIC") : activePrivacy_);
    if (!hasActiveSession()) {
        setError(static_cast<int>(astra::common::ErrorCode::ProjectionSessionMissing),
                 registeredErrorMessage(astra::common::ErrorCode::ProjectionSessionMissing));
        writeAudit(QStringLiteral("projection_error"), intent, QStringLiteral("failed"), lastErrorCode_);
    } else {
        projectionModel_.setView(projectionModel_.sceneRotation() + delta, projectionModel_.zoom());
        writeAudit(QStringLiteral("view_rotated"), intent, QStringLiteral("success"));
    }
    emit taskChanged();
}

void ShellController::enterFullscreen()
{
    if (projectionFullscreen_) return;
    projectionFullscreen_ = true;
    const SimulatedIntent intent = intentSimulator_.parse(QStringLiteral("进入全屏"), activePrivacy_.isEmpty() ? QStringLiteral("PUBLIC") : activePrivacy_);
    writeAudit(QStringLiteral("fullscreen_entered"), intent, QStringLiteral("success"));
    emit fullscreenChanged();
}

void ShellController::exitFullscreen()
{
    if (!projectionFullscreen_) return;
    projectionFullscreen_ = false;
    const SimulatedIntent intent = intentSimulator_.parse(QStringLiteral("退出全屏"), activePrivacy_.isEmpty() ? QStringLiteral("PUBLIC") : activePrivacy_);
    writeAudit(QStringLiteral("fullscreen_exited"), intent, QStringLiteral("success"));
    emit fullscreenChanged();
}

} // namespace astra::shell
