#pragma once

#include "astra/ui/NotificationSurface.h"
#include "astra/ui/InputRouter.h"
#include "astra/ui/InteractionStateMachine.h"
#include "astra/ui/PrivacyAwareUIService.h"
#include "astra/ui/SpatialFocusManager.h"
#include "astra/ui/SpatialLayoutEngine.h"
#include "astra/ui/SpatialWindowManager.h"
#include "astra/ui/TaskSurface.h"
#include "astra/ui/UIComponentRegistry.h"
#include "astra/ui/UIStateStore.h"
#include "astra/spatial_ui/service/ProjectionGateway.h"

#include <QJsonObject>

#include <memory>
#include <QPointF>
#include <QVector>

namespace astra::spatial_ui::service {

struct RuntimeResponse {
    bool ok {false};
    int errorCode {0};
    QString message;
    QJsonObject value;
};

struct SpatialUIRuntimeOptions {
    int maximumComponents {500};
    int maximumWindows {20};
    bool statePersistence {true};
    int autosaveIntervalSeconds {10};
    bool mouseEnabled {true};
    bool keyboardEnabled {true};
    bool touchEnabled {true};
    bool simulatedGestureEnabled {true};
    bool accessibilityEnabled {true};
    bool keyboardNavigationEnabled {true};
    bool visibleFocus {true};
    bool reduceMotion {false};
    bool highContrast {false};
    bool roomTrusted {false};
    bool authorizedPersonPresent {false};
    bool projectionTargetEnabled {false};
    astra::common::PrivacyLevel maximumFixtureDisclosure {astra::common::PrivacyLevel::PrivateScreenOnly};
    QString publicFixtureSubjectId;
    QString policyBindingKey;
    astra::ui::LayoutMode defaultLayout {astra::ui::LayoutMode::Stack};
    double layoutSpacing {16.0};
    double layoutMargin {24.0};
    QString configurationStatus {QStringLiteral("SAFE_DEFAULTS")};
    QString configurationWarning {QStringLiteral("Runtime configuration was not supplied; projection is disabled")};
};

class SpatialUIRuntime final {
public:
    explicit SpatialUIRuntime(QString statePath,
                              std::shared_ptr<ProjectionGateway> projectionGateway = {},
                              SpatialUIRuntimeOptions options = {});
    RuntimeResponse dispatch(const QString &method,
                             const QJsonObject &params,
                             const QString &traceId = {},
                             const QString &requestId = {});
    RuntimeResponse performMaintenance(const QDateTime &now,
                                       const QString &traceId,
                                       const QString &requestId,
                                       bool autosaveDue);
    [[nodiscard]] QJsonObject status() const;

private:
    RuntimeResponse createComponent(const QJsonObject &params);
    RuntimeResponse createTaskSurface(const QJsonObject &params);
    RuntimeResponse updateTaskSurface(const QJsonObject &params);
    RuntimeResponse updateComponent(const QJsonObject &params);
    RuntimeResponse removeComponent(const QJsonObject &params);
    RuntimeResponse openWindow(const QJsonObject &params);
    RuntimeResponse updateWindow(const QString &method, const QJsonObject &params);
    RuntimeResponse applyLayout(const QJsonObject &params);
    RuntimeResponse createNotification(const QJsonObject &params);
    RuntimeResponse clearNotification(const QJsonObject &params);
    RuntimeResponse expireNotifications(const QDateTime &now, const QString &traceId, const QString &requestId);
    RuntimeResponse saveState();
    [[nodiscard]] QVector<astra::render::ProjectionLayer> projectionLayers() const;
    RuntimeResponse synchronizeProjection(const QString &componentId, const QString &traceId, const QString &requestId);
    RuntimeResponse cancelInteraction(const QJsonObject &params);
    RuntimeResponse applyWindowInteraction(const QString &method, const QJsonObject &params,
                                           astra::ui::SpatialWindow &window, astra::ui::SpatialUIComponent &component);
    RuntimeResponse applyInputInteraction(const astra::ui::InputEvent &event, const QString &targetComponentId);
    [[nodiscard]] static QString interactionSourceKey(const astra::ui::InputEvent &event);
    void clearInteraction(const QString &componentId);
    astra::ui::OperationResult cancelInteractionForUnavailableComponent(const QString &componentId);
    astra::ui::OperationResult synchronizeWindowBounds(const QString &componentId);
    [[nodiscard]] astra::policy::ProjectionPolicyDecision classifyFixture(
        const QString &subjectId, astra::common::PrivacyLevel requestedLevel) const;

    astra::ui::UIComponentRegistry components_;
    astra::ui::SpatialWindowManager windows_;
    astra::ui::SpatialLayoutEngine layout_;
    astra::ui::SpatialFocusManager focus_;
    astra::ui::InputRouter input_;
    astra::ui::TaskSurfaceManager tasks_;
    astra::ui::NotificationSurfaceManager notifications_;
    astra::ui::PrivacyAwareUIService privacy_;
    astra::ui::ProjectionLayerMapper layerMapper_;
    astra::ui::UIStateStore stateStore_;
    std::shared_ptr<ProjectionGateway> projectionGateway_;
    SpatialUIRuntimeOptions options_;
    bool targetAvailable_ {true};
    bool projectionSafe_ {true};
    bool projectionContentActive_ {false};
    quint64 lastProjectionFrameId_ {0};
    astra::ui::LayoutMode layoutMode_ {astra::ui::LayoutMode::Stack};
    struct CriticalFocusRestore {
        QString notificationId;
        astra::ui::FocusRecord previous;
    };
    QVector<CriticalFocusRestore> criticalFocusRestoreChain_;
    QHash<QString, std::shared_ptr<astra::ui::InteractionSession>> interactionSessions_;
    QHash<QString, QPointF> interactionStartPositions_;
    QHash<QString, QString> interactionTargetsBySource_;
    QString selectedTab_ {QStringLiteral("tasks")};
    QStringList panelOrder_ {QStringLiteral("tasks"), QStringLiteral("system")};
};

} // namespace astra::spatial_ui::service
