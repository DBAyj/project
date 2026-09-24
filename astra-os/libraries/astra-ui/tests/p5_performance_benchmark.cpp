#include "astra/ui/InputRouter.h"
#include "astra/ui/PrivacyAwareUIService.h"
#include "astra/ui/SpatialFocusManager.h"
#include "astra/ui/SpatialLayoutEngine.h"
#include "astra/ui/SpatialUIComponent.h"

#include <QCoreApplication>
#include <QElapsedTimer>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QtGlobal>

#include <algorithm>
#include <numeric>
#include <vector>

using namespace astra::ui;

namespace {

constexpr auto kBaselineMarker = "P4_RELEASE_BASELINE_FINAL";

void require(bool condition, const char *message)
{
    if (!condition) qFatal("P5 performance benchmark failed: %s", message);
}

template<typename Function>
std::vector<double> measure(int iterations, Function operation)
{
    std::vector<double> samples;
    samples.reserve(static_cast<std::size_t>(iterations));
    for (int index = 0; index < iterations; ++index) {
        QElapsedTimer timer;
        timer.start();
        operation(index);
        samples.push_back(static_cast<double>(timer.nsecsElapsed()) / 1'000'000.0);
    }
    return samples;
}

double percentile95(std::vector<double> samples)
{
    std::sort(samples.begin(), samples.end());
    const auto index = std::min(samples.size() - 1, static_cast<std::size_t>(samples.size() * 0.95));
    return samples.at(index);
}

double mean(const std::vector<double> &samples)
{
    return std::accumulate(samples.begin(), samples.end(), 0.0) / static_cast<double>(samples.size());
}

ComponentSpec componentSpec(int index)
{
    ComponentSpec spec;
    spec.id = QStringLiteral("benchmark-%1").arg(index);
    spec.type = ComponentType::TaskCard;
    spec.bounds = {0.0, 0.0, 120.0, 72.0};
    spec.zOrder = index;
    spec.focusable = true;
    spec.interactive = true;
    spec.privacyLevel = astra::common::PrivacyLevel::Public;
    spec.accessibilityLabel = QStringLiteral("Performance fixture %1").arg(index);
    return spec;
}

} // namespace

int main(int argc, char *argv[])
{
    QCoreApplication application(argc, argv);
    require(argc == 2, "expected output JSON path");

    LayoutRequest layoutRequest;
    layoutRequest.mode = LayoutMode::Grid;
    layoutRequest.safeArea = {0.0, 0.0, 1920.0, 1080.0};
    layoutRequest.margin = 24.0;
    layoutRequest.spacing = 8.0;
    layoutRequest.columns = 10;
    for (int index = 0; index < 100; ++index) {
        layoutRequest.items.append({QStringLiteral("component-%1").arg(index), {0.0, 0.0, 120.0, 72.0}});
    }
    SpatialLayoutEngine layout;
    const auto layoutSamples = measure(1'000, [&](int) {
        const auto result = layout.apply(layoutRequest);
        require(result.ok && result.placements.size() == 100, "100-component layout");
    });

    QList<HitTarget> targets;
    for (int index = 0; index < 100; ++index) {
        const int column = index % 10;
        const int row = index / 10;
        targets.append({QStringLiteral("target-%1").arg(index),
                        {static_cast<double>(column * 120), static_cast<double>(row * 80), 110.0, 70.0},
                        HitShape::Rectangle, {}, index, false, false, true, true, 1.0});
    }
    InputRouter input;
    input.setTargets(targets);
    const auto inputSamples = measure(10'000, [&](int index) {
        const int target = index % 100;
        const InputEvent event {QStringLiteral("event-%1").arg(index), InputEventType::PointerMove, InputSourceType::Mouse,
                                QStringLiteral("performance-mouse"), CoordinateSystem::ProjectionView,
                                static_cast<double>((target % 10) * 120 + 20), static_cast<double>((target / 10) * 80 + 20)};
        require(input.route(event).ok, "input route");
    });

    SpatialFocusManager focus;
    for (int index = 0; index < 100; ++index) {
        require(focus.registerTarget({QStringLiteral("focus-%1").arg(index), QStringLiteral("benchmark"), true, true, true, index}).ok,
                "focus target registration");
    }
    const auto focusSamples = measure(5'000, [&](int) {
        require(focus.focusNext(QStringLiteral("benchmark")).ok, "focus traversal");
    });

    SpatialUIComponent stateComponent {componentSpec(0)};
    const auto stateSamples = measure(5'000, [&](int index) {
        require(stateComponent.setBounds({static_cast<double>(index % 800), static_cast<double>(index % 400), 120.0, 72.0}).ok,
                "component state update");
    });

    std::vector<SpatialUIComponent> components;
    components.reserve(100);
    for (int index = 0; index < 100; ++index) {
        components.emplace_back(componentSpec(index));
        require(components.back().transitionTo(ComponentLifecycleState::Attached).ok, "component attach");
        require(components.back().transitionTo(ComponentLifecycleState::Visible).ok, "component show");
    }
    PrivacyAwareUIService privacy;
    const QString policyBindingKey = QStringLiteral("performance-policy-binding-key");
    ProjectionLayerMapper mapper {privacy, policyBindingKey};
    const auto layerSamples = measure(1'000, [&](int) {
        int generated = 0;
        for (const auto &component : components) {
            astra::policy::ProjectionPolicyRequest request {astra::common::PrivacyLevel::Public};
            request.subjectId = component.componentId();
            generated += mapper.map(component, astra::policy::ProjectionPolicyService::evaluate(request, policyBindingKey)).has_value() ? 1 : 0;
        }
        require(generated == 100, "100-layer generation");
    });

    const auto frameSamples = measure(600, [&](int index) {
        const auto result = layout.apply(layoutRequest);
        require(result.ok, "frame layout");
        require(stateComponent.setBounds({static_cast<double>(index), 0.0, 120.0, 72.0}).ok, "frame state update");
        for (const auto &component : components) {
            astra::policy::ProjectionPolicyRequest request {astra::common::PrivacyLevel::Public};
            request.subjectId = component.componentId();
            require(mapper.map(component, astra::policy::ProjectionPolicyService::evaluate(request, policyBindingKey)).has_value(), "frame layer generation");
        }
    });

    QElapsedTimer firstFrameTimer;
    firstFrameTimer.start();
    const auto firstLayout = layout.apply(layoutRequest);
    int firstLayers = 0;
    for (const auto &component : components) {
        astra::policy::ProjectionPolicyRequest request {astra::common::PrivacyLevel::Public};
        request.subjectId = component.componentId();
        firstLayers += mapper.map(component, astra::policy::ProjectionPolicyService::evaluate(request, policyBindingKey)).has_value() ? 1 : 0;
    }
    const double frameworkFirstFrameMs = static_cast<double>(firstFrameTimer.nsecsElapsed()) / 1'000'000.0;
    require(firstLayout.ok && firstLayers == 100, "framework first frame");

    const double frameP95 = percentile95(frameSamples);
    const QJsonObject metrics {
        {QStringLiteral("schema_version"), QStringLiteral("1.0")},
        {QStringLiteral("p4_release_status"), QString::fromLatin1(kBaselineMarker)},
        {QStringLiteral("visible_components"), 100},
        {QStringLiteral("layout_iterations"), static_cast<int>(layoutSamples.size())},
        {QStringLiteral("input_iterations"), static_cast<int>(inputSamples.size())},
        {QStringLiteral("focus_iterations"), static_cast<int>(focusSamples.size())},
        {QStringLiteral("layer_iterations"), static_cast<int>(layerSamples.size())},
        {QStringLiteral("frame_iterations"), static_cast<int>(frameSamples.size())},
        {QStringLiteral("layout_p95_ms"), percentile95(layoutSamples)},
        {QStringLiteral("input_p95_ms"), percentile95(inputSamples)},
        {QStringLiteral("focus_p95_ms"), percentile95(focusSamples)},
        {QStringLiteral("state_update_p95_ms"), percentile95(stateSamples)},
        {QStringLiteral("projection_layer_p95_ms"), percentile95(layerSamples)},
        {QStringLiteral("framework_first_frame_ms"), frameworkFirstFrameMs},
        {QStringLiteral("mean_fps"), 1'000.0 / mean(frameSamples)},
        {QStringLiteral("stable_fps_floor"), 1'000.0 / frameP95},
    };
    QFile output {QString::fromLocal8Bit(argv[1])};
    require(output.open(QIODevice::WriteOnly | QIODevice::Truncate), "open output JSON");
    require(output.write(QJsonDocument {metrics}.toJson(QJsonDocument::Indented)) > 0, "write output JSON");
    return 0;
}
