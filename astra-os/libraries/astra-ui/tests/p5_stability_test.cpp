#include "astra/ui/InputRouter.h"
#include "astra/ui/InteractionStateMachine.h"
#include "astra/ui/NotificationSurface.h"
#include "astra/ui/PrivacyAwareUIService.h"
#include "astra/ui/SpatialFocusManager.h"
#include "astra/ui/SpatialLayoutEngine.h"
#include "astra/ui/SpatialWindowManager.h"
#include "astra/ui/UIComponentRegistry.h"
#include "astra/ui/UIStateStore.h"

#include <QCoreApplication>
#include <QDateTime>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTemporaryDir>
#include <QtGlobal>

#if defined(Q_OS_MACOS)
#include <mach/mach.h>
#elif defined(Q_OS_LINUX)
#include <unistd.h>
#endif

using namespace astra::ui;

namespace {

constexpr auto kBaselineMarker = "P4_RELEASE_BASELINE_FINAL";

void require(bool condition, const char *message)
{
    if (!condition) qFatal("P5 stability test failed: %s", message);
}

qint64 residentBytes()
{
#if defined(Q_OS_MACOS)
    mach_task_basic_info_data_t info {};
    mach_msg_type_number_t count = MACH_TASK_BASIC_INFO_COUNT;
    if (task_info(mach_task_self(), MACH_TASK_BASIC_INFO, reinterpret_cast<task_info_t>(&info), &count) != KERN_SUCCESS) return -1;
    return static_cast<qint64>(info.resident_size);
#elif defined(Q_OS_LINUX)
    QFile statm {QStringLiteral("/proc/self/statm")};
    if (!statm.open(QIODevice::ReadOnly)) return -1;
    const QList<QByteArray> values = statm.readAll().simplified().split(' ');
    if (values.size() < 2) return -1;
    return values.at(1).toLongLong() * static_cast<qint64>(sysconf(_SC_PAGESIZE));
#else
    return -1;
#endif
}

ComponentSpec componentSpec(const QString &id, astra::common::PrivacyLevel privacy = astra::common::PrivacyLevel::Public)
{
    ComponentSpec spec;
    spec.id = id;
    spec.type = ComponentType::TaskCard;
    spec.bounds = {40.0, 40.0, 320.0, 180.0};
    spec.focusable = true;
    spec.interactive = true;
    spec.privacyLevel = privacy;
    spec.accessibilityLabel = QStringLiteral("Stability fixture %1").arg(id);
    return spec;
}

void destroy(UIComponentRegistry &registry, SpatialFocusManager &focus, const QString &id)
{
    auto *component = registry.find(id);
    require(component != nullptr, "component exists before destroy");
    if (component->lifecycleState() == ComponentLifecycleState::Focused) require(component->transitionTo(ComponentLifecycleState::Visible).ok, "defocus component");
    if (component->isVisible()) require(component->transitionTo(ComponentLifecycleState::Hidden).ok, "hide component");
    require(component->transitionTo(ComponentLifecycleState::Detached).ok, "detach component");
    require(component->transitionTo(ComponentLifecycleState::Destroyed).ok, "destroy component");
    require(focus.unregisterTarget(id).ok, "unregister destroyed focus target");
    require(registry.remove(id).ok, "remove destroyed component");
}

} // namespace

int main(int argc, char *argv[])
{
    QCoreApplication application(argc, argv);
    require(argc == 2, "expected output JSON path");

    InputRouter input;
    input.setTargets({{QStringLiteral("surface"), {0.0, 0.0, 1280.0, 720.0}, HitShape::Rectangle, {}, 1, false, false, true, true, 1.0}});
    for (int index = 0; index < 10'000; ++index) {
        const InputEvent event {QStringLiteral("stability-input-%1").arg(index), InputEventType::PointerMove, InputSourceType::Mouse,
                                QStringLiteral("stability-mouse"), CoordinateSystem::ProjectionView,
                                static_cast<double>(index % 1280), static_cast<double>(index % 720)};
        require(input.route(event).ok, "10,000 input events");
    }

    SpatialFocusManager focus;
    for (int index = 0; index < 20; ++index) {
        require(focus.registerTarget({QStringLiteral("stable-focus-%1").arg(index), QStringLiteral("stability"), true, true, true, index}).ok,
                "focus registration");
    }
    for (int index = 0; index < 5'000; ++index) require(focus.focusNext(QStringLiteral("stability")).ok, "5,000 focus switches");
    require(focus.releaseFocus(QStringLiteral("stability"), QStringLiteral("stability complete")).ok, "release traversal focus");

    UIComponentRegistry registry {32};
    SpatialFocusManager lifecycleFocus;
    QList<qint64> rssCheckpoints;
    for (int index = 0; index < 2'000; ++index) {
        const QString id = QStringLiteral("lifecycle-%1").arg(index);
        auto created = registry.create(componentSpec(id), Principal::Application);
        require(created.ok, "2,000 component creates");
        require(created.component->transitionTo(ComponentLifecycleState::Attached).ok, "attach lifecycle component");
        require(created.component->transitionTo(ComponentLifecycleState::Visible).ok, "show lifecycle component");
        require(lifecycleFocus.registerTarget({id, QStringLiteral("lifecycle"), true, true, true, index}).ok, "register lifecycle focus");
        require(lifecycleFocus.requestFocus(id, FocusType::Keyboard, FocusPriority::ActiveWindow, QStringLiteral("stability lifecycle")).ok,
                "focus lifecycle component");
        destroy(registry, lifecycleFocus, id);
        require(lifecycleFocus.owner(QStringLiteral("lifecycle")).isEmpty(), "destroyed component cannot retain focus");
        if ((index + 1) % 500 == 0) rssCheckpoints.append(residentBytes());
    }
    require(rssCheckpoints.size() == 4 && std::all_of(rssCheckpoints.begin(), rssCheckpoints.end(), [](qint64 value) { return value > 0; }),
            "resident memory sampling");
    const qint64 rssGrowth = std::max<qint64>(0, rssCheckpoints.last() - rssCheckpoints.first());
    const qint64 rssSecondHalfGrowth = std::max<qint64>(0, rssCheckpoints.last() - rssCheckpoints.at(1));

    SpatialWindowManager windows {20, {0.0, 0.0, 1280.0, 720.0}};
    SpatialWindowSpec windowSpec;
    windowSpec.windowId = QStringLiteral("stability-window");
    windowSpec.componentId = QStringLiteral("stability-window-component");
    windowSpec.bounds = {100.0, 100.0, 400.0, 240.0};
    windowSpec.focusScope = QStringLiteral("stability");
    require(windows.createWindow(windowSpec).ok && windows.showWindow(windowSpec.windowId).ok, "create stability window");
    for (int index = 0; index < 1'000; ++index) {
        require(windows.moveWindow(windowSpec.windowId, static_cast<double>((index * 7) % 1400), static_cast<double>((index * 5) % 800)).ok,
                "1,000 window moves");
    }
    for (int index = 0; index < 1'000; ++index) {
        require(windows.resizeWindow(windowSpec.windowId, 160.0 + static_cast<double>(index % 900), 100.0 + static_cast<double>(index % 500)).ok,
                "1,000 window resizes");
    }
    const auto finalWindowBounds = windows.find(windowSpec.windowId)->bounds();
    require(finalWindowBounds.x >= 0.0 && finalWindowBounds.y >= 0.0
                && finalWindowBounds.x + finalWindowBounds.width <= 1280.0
                && finalWindowBounds.y + finalWindowBounds.height <= 720.0,
            "window remains in safe area");

    SpatialLayoutEngine layout;
    LayoutRequest layoutRequest;
    layoutRequest.safeArea = {0.0, 0.0, 1280.0, 720.0};
    layoutRequest.anchor = {400.0, 220.0, 20.0, 20.0};
    layoutRequest.columns = 5;
    for (int index = 0; index < 50; ++index) {
        layoutRequest.items.append({QStringLiteral("layout-%1").arg(index), {0.0, 0.0, 120.0, 72.0}});
    }
    const QList<LayoutMode> modes {LayoutMode::Stack, LayoutMode::Grid, LayoutMode::Radial, LayoutMode::Freeform, LayoutMode::AnchorRelative};
    for (int index = 0; index < 500; ++index) {
        layoutRequest.mode = modes.at(index % modes.size());
        require(layout.apply(layoutRequest).ok, "500 layout switches");
    }

    NotificationSurfaceManager notifications;
    for (int index = 0; index < 500; ++index) {
        const QDateTime createdAt = QDateTime::currentDateTimeUtc();
        NotificationSpec notification {QStringLiteral("notification-%1").arg(index), QStringLiteral("Stability"), QStringLiteral("Expiring fixture"),
                                       NotificationSeverity::Warning, astra::common::PrivacyLevel::Public, DisplayTarget::Both,
                                       1, false, {}, createdAt};
        require(notifications.create(notification).ok, "500 notification creates");
        require(notifications.expire(createdAt.addMSecs(2)) == 1, "500 notification expirations");
    }

    QTemporaryDir stateDirectory;
    require(stateDirectory.isValid(), "temporary state directory");
    UIStateStore stateStore {stateDirectory.filePath(QStringLiteral("ui-state.json"))};
    for (int index = 0; index < 100; ++index) {
        UIStateSnapshot snapshot;
        snapshot.layoutMode = modes.at(index % modes.size());
        snapshot.windows = {{QStringLiteral("stability-window"), QStringLiteral("stability-window-component"),
                             finalWindowBounds, true, DisplayTarget::Phone, {}, {}, QStringLiteral("stability")}};
        snapshot.visibleComponentIds = {QStringLiteral("surface")};
        snapshot.selectedTab = QStringLiteral("tasks");
        snapshot.panelOrder = {QStringLiteral("tasks"), QStringLiteral("system")};
        require(stateStore.save(snapshot).ok, "100 state saves");
        const auto loaded = stateStore.load();
        require(loaded.ok && loaded.snapshot.windows.size() == 1, "100 state restores");
    }

    for (int index = 0; index < 100; ++index) {
        layoutRequest.safeArea = index % 2 == 0 ? SpatialBounds {0.0, 0.0, 1280.0, 720.0} : SpatialBounds {0.0, 0.0, 1920.0, 1080.0};
        require(layout.apply(layoutRequest).ok, "100 fullscreen safe-area recalculations");
    }

    PrivacyAwareUIService privacy;
    const QString policyBindingKey = QStringLiteral("stability-policy-binding-key");
    ProjectionLayerMapper mapper {privacy, policyBindingKey};
    int privacyLeaks = 0;
    for (int index = 0; index < 100; ++index) {
        SpatialUIComponent publicComponent {componentSpec(QStringLiteral("chain-public-%1").arg(index))};
        SpatialUIComponent privateComponent {componentSpec(QStringLiteral("chain-private-%1").arg(index), astra::common::PrivacyLevel::PrivateScreenOnly)};
        require(publicComponent.transitionTo(ComponentLifecycleState::Attached).ok
                    && publicComponent.transitionTo(ComponentLifecycleState::Visible).ok,
                "fixture chain public component lifecycle");
        require(privateComponent.transitionTo(ComponentLifecycleState::Attached).ok
                    && privateComponent.transitionTo(ComponentLifecycleState::Visible).ok,
                "fixture chain private component lifecycle");
        astra::policy::ProjectionPolicyRequest publicRequest {astra::common::PrivacyLevel::Public};
        publicRequest.subjectId = publicComponent.componentId();
        astra::policy::ProjectionPolicyRequest privateRequest {astra::common::PrivacyLevel::PrivateScreenOnly};
        privateRequest.subjectId = privateComponent.componentId();
        require(mapper.map(publicComponent, astra::policy::ProjectionPolicyService::evaluate(publicRequest, policyBindingKey)).has_value(),
                "fixture chain public P4 layer");
        if (mapper.map(privateComponent, astra::policy::ProjectionPolicyService::evaluate(privateRequest, policyBindingKey)).has_value()) ++privacyLeaks;
        InteractionSession interaction {publicComponent, {0.0, 0.0, 1280.0, 720.0}};
        require(interaction.press().ok && interaction.begin(InteractionState::Dragging).ok
                    && interaction.dragBy(20.0, 10.0).ok && interaction.commit().ok,
                "100 P1-P5 fixture interaction cycles");
    }

    require(registry.size() == 0, "no retained lifecycle components");
    require(notifications.size() == 0, "no retained notifications");
    require(lifecycleFocus.owner(QStringLiteral("lifecycle")).isEmpty(), "no residual lifecycle focus");
    require(privacyLeaks == 0, "no privacy component leakage");

    const QJsonObject result {
        {QStringLiteral("schema_version"), QStringLiteral("1.0")},
        {QStringLiteral("p4_release_status"), QString::fromLatin1(kBaselineMarker)},
        {QStringLiteral("input_events"), 10'000},
        {QStringLiteral("focus_switches"), 5'000},
        {QStringLiteral("component_create_destroy_cycles"), 2'000},
        {QStringLiteral("window_moves"), 1'000},
        {QStringLiteral("window_resizes"), 1'000},
        {QStringLiteral("layout_switches"), 500},
        {QStringLiteral("notification_expiry_cycles"), 500},
        {QStringLiteral("state_save_restore_cycles"), 100},
        {QStringLiteral("fullscreen_layout_cycles"), 100},
        {QStringLiteral("p1_p5_fixture_interaction_cycles"), 100},
        {QStringLiteral("retained_components"), static_cast<int>(registry.size())},
        {QStringLiteral("retained_notifications"), static_cast<int>(notifications.size())},
        {QStringLiteral("residual_focus"), !lifecycleFocus.owner(QStringLiteral("lifecycle")).isEmpty()},
        {QStringLiteral("privacy_leaks"), privacyLeaks},
        {QStringLiteral("rss_after_500_bytes"), rssCheckpoints.first()},
        {QStringLiteral("rss_after_2000_bytes"), rssCheckpoints.last()},
        {QStringLiteral("rss_growth_bytes"), rssGrowth},
        {QStringLiteral("rss_second_half_growth_bytes"), rssSecondHalfGrowth},
        {QStringLiteral("result"), QStringLiteral("P5_STABILITY_PASSED")},
    };
    QFile output {QString::fromLocal8Bit(argv[1])};
    require(output.open(QIODevice::WriteOnly | QIODevice::Truncate), "open stability output");
    require(output.write(QJsonDocument {result}.toJson(QJsonDocument::Indented)) > 0, "write stability output");
    return 0;
}
