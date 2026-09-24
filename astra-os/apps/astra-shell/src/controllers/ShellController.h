#pragma once

#include "clients/ProjectionServiceClient.h"
#include "clients/SpatialUIServiceClient.h"
#include "clients/IntentServiceClient.h"
#include "clients/SpatialServiceClient.h"
#include "models/IntentResultModel.h"
#include "models/ProjectionStateModel.h"
#include "models/SpatialUIStateModel.h"
#include "models/SystemStateModel.h"
#include "models/SpatialStateModel.h"
#include "models/TaskModel.h"
#include "services/AuditLogService.h"
#include "services/ConfigurationService.h"
#include "services/IntentSimulator.h"
#include "services/ProjectionPolicyService.h"
#include "services/ProjectionSessionService.h"

#include <QObject>
#include <QJsonObject>
#include <QTimer>
#include <optional>

namespace astra::shell {

class ShellController final : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString projectionState READ projectionState NOTIFY stateChanged)
    Q_PROPERTY(QString taskResult READ taskResult NOTIFY taskChanged)
    Q_PROPERTY(QString errorMessage READ errorMessage NOTIFY taskChanged)
    Q_PROPERTY(int lastErrorCode READ lastErrorCode NOTIFY taskChanged)
    Q_PROPERTY(QString currentRawText READ currentRawText NOTIFY taskChanged)
    Q_PROPERTY(QString currentIntent READ currentIntent NOTIFY taskChanged)
    Q_PROPERTY(QString currentTargetSpace READ currentTargetSpace NOTIFY taskChanged)
    Q_PROPERTY(QString currentPrivacyLevel READ currentPrivacyLevel NOTIFY taskChanged)
    Q_PROPERTY(QString currentTaskStatus READ currentTaskStatus NOTIFY taskChanged)
    Q_PROPERTY(QString currentRequestId READ currentRequestId NOTIFY taskChanged)
    Q_PROPERTY(bool projectionFullscreen READ projectionFullscreen NOTIFY fullscreenChanged)
    Q_PROPERTY(bool p4ProjectionActive READ p4ProjectionActive NOTIFY stateChanged)
    Q_PROPERTY(QString p4FramePath READ p4FramePath NOTIFY stateChanged)
    Q_PROPERTY(SystemStateModel *systemState READ systemState CONSTANT)
    Q_PROPERTY(TaskModel *recentTasks READ recentTasks CONSTANT)
    Q_PROPERTY(ProjectionStateModel *projectionModel READ projectionModel CONSTANT)
    Q_PROPERTY(SpatialUIStateModel *spatialUIModel READ spatialUIModel CONSTANT)
    Q_PROPERTY(bool spatialUIAvailable READ spatialUIAvailable NOTIFY spatialUIChanged)
    Q_PROPERTY(IntentResultModel *intentResult READ intentResult CONSTANT)
    Q_PROPERTY(SpatialStateModel *spatialState READ spatialState CONSTANT)

public:
    explicit ShellController(const SimulatorConfig &config = {},
                             QString auditPath = QStringLiteral("runtime/audit/audit.jsonl"),
                             QObject *parent = nullptr);
    ShellController(const SimulatorConfig &config,
                    QString auditPath,
                    IntentServiceClient intentClient,
                    bool p2Mode,
                    QObject *parent = nullptr);

    QString projectionState() const;
    QString taskResult() const;
    QString errorMessage() const;
    int lastErrorCode() const;
    QString currentRawText() const;
    QString currentIntent() const;
    QString currentTargetSpace() const;
    QString currentPrivacyLevel() const;
    QString currentTaskStatus() const;
    QString currentRequestId() const;
    bool projectionFullscreen() const;
    bool p4ProjectionActive() const;
    QString p4FramePath() const;
    SystemStateModel *systemState();
    TaskModel *recentTasks();
    ProjectionStateModel *projectionModel();
    SpatialUIStateModel *spatialUIModel();
    bool spatialUIAvailable() const;
    IntentResultModel *intentResult();
    SpatialStateModel *spatialState();

    Q_INVOKABLE void submit(const QString &text, const QString &privacy);
    Q_INVOKABLE void stopProjection();
    Q_INVOKABLE void pauseRotation();
    Q_INVOKABLE void resumeRotation();
    Q_INVOKABLE void resetView();
    Q_INVOKABLE void changeZoom(double delta);
    Q_INVOKABLE void rotateView(double delta);
    Q_INVOKABLE void enterFullscreen();
    Q_INVOKABLE void exitFullscreen();
    Q_INVOKABLE void refreshSpatialUI();
    Q_INVOKABLE void createSpatialTaskCard(const QString &title, const QString &summary, const QString &privacyLevel);
    Q_INVOKABLE void activateSpatialComponent(const QString &componentId, const QString &action);
    Q_INVOKABLE void routeSpatialInput(const QString &componentId,
                                       const QString &eventType,
                                       const QString &sourceType,
                                       const QString &coordinateSystem,
                                       double x,
                                       double y);
    Q_INVOKABLE void confirmIntent();
    Q_INVOKABLE void rejectIntent();
    Q_INVOKABLE void startSpatial();
    Q_INVOKABLE void refreshSpatial();
    Q_INVOKABLE void detectSpatial();
    Q_INVOKABLE void selectSpatial();
    Q_INVOKABLE void calibrateSpatial();
    Q_INVOKABLE void resetSpatial();
    void applySpatialState(const QJsonObject &state);
    void setConfigurationStatus(const QString &status);
    void setConfigurationDiagnostics(const QStringList &warnings, bool usingFallback);
    void recordApplicationStarted();
    void recordApplicationStopped();
    void recordConfigurationResult(bool valid);
    void recordApplicationInitializationFailure();
    void startP4FixtureDemo();

signals:
    void stateChanged();
    void taskChanged();
    void fullscreenChanged();
    void spatialUIChanged();

private:
    void applyProjectionResult(const SimulatedIntent &intent,
                               const ProjectionOperationResult &result,
                               const QString &successEvent,
                               const QString &failureEvent);
    void addTask(const SimulatedIntent &intent, const QString &status);
    void clearError();
    void setError(int code, const QString &message);
    void writeAudit(const QString &event,
                    const SimulatedIntent &intent,
                    const QString &result,
                    int errorCode = 0,
                    const QJsonObject &details = {});
    SimulatedIntent lifecycleIntent(const QString &intent) const;
    bool hasActiveSession() const;
    QString activeProjectionState() const;
    QString activeProjectionSessionId() const;
    ProjectionOperationResult startProjectionRuntime();
    ProjectionOperationResult pauseProjectionRuntime();
    ProjectionOperationResult resumeProjectionRuntime();
    ProjectionOperationResult stopProjectionRuntime();
    void executeIntent(const SimulatedIntent &intent);
    SimulatedIntent serviceIntent(const QString &text,
                                  const QString &selectedPrivacy,
                                  const IntentServiceResult &result) const;
    void presentNonExecutableIntent(const SimulatedIntent &intent, const IntentServiceResult &result);
    void invokeSpatial(const QString &method, const QJsonObject &params = {});

    IntentSimulator intentSimulator_;
    IntentServiceClient intentClient_;
    SpatialServiceClient spatialClient_;
    IntentResultModel intentResultModel_;
    SystemStateModel systemState_;
    SpatialStateModel spatialState_;
    TaskModel taskModel_;
    ProjectionStateModel projectionModel_;
    ProjectionSessionService projectionService_;
    ProjectionServiceClient projectionServiceClient_;
    SpatialUIServiceClient spatialUIServiceClient_;
    SpatialUIStateModel spatialUIModel_;
    AuditLogService audit_;
    QString taskResult_ {QStringLiteral("等待任务")};
    QString errorMessage_;
    QString currentRawText_;
    QString currentIntent_;
    QString currentTargetSpace_;
    QString currentPrivacyLevel_;
    QString currentTaskStatus_ {QStringLiteral("IDLE")};
    QString currentRequestId_;
    QString activePrivacy_;
    QString activeTarget_;
    QString activeSessionId_;
    bool simulatedAuthorized_ {false};
    int lastErrorCode_ {0};
    bool projectionFullscreen_ {false};
    bool p4ProjectionActive_ {false};
    bool p2Mode_ {false};
    std::optional<SimulatedIntent> pendingIntent_;
    QTimer spatialStatePollTimer_;
    QTimer spatialUIPollTimer_;
};

} // namespace astra::shell
