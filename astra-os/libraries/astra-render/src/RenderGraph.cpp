#include "astra/render/RenderGraph.h"

#include "astra/common/ErrorCode.h"

#include <QHash>

#include <functional>

namespace astra::render {

RenderGraph::RenderGraph(QVector<RenderPass> passes)
    : passes_(std::move(passes))
{
}

RenderGraphResult RenderGraph::topologicalOrder() const
{
    QHash<QString, RenderPass> passById;
    for (const auto &pass : passes_) {
        if (pass.id.isEmpty() || passById.contains(pass.id)) {
            return {false, static_cast<int>(astra::common::ErrorCode::RenderGraphDependencyMissing), {},
                    QStringLiteral("Render graph pass identifier is invalid")};
        }
        passById.insert(pass.id, pass);
    }

    enum class Mark { Unseen, Visiting, Complete };
    QHash<QString, Mark> marks;
    QStringList order;
    std::function<RenderGraphResult(const QString &)> visit = [&](const QString &id) -> RenderGraphResult {
        if (!passById.contains(id)) {
            return {false, static_cast<int>(astra::common::ErrorCode::RenderGraphDependencyMissing), {},
                    QStringLiteral("Render graph dependency is missing")};
        }
        const auto mark = marks.value(id, Mark::Unseen);
        if (mark == Mark::Visiting) {
            return {false, static_cast<int>(astra::common::ErrorCode::RenderGraphCycleDetected), {},
                    QStringLiteral("Render graph contains a cycle")};
        }
        if (mark == Mark::Complete) return {true, 0, {}, {}};

        marks.insert(id, Mark::Visiting);
        const auto pass = passById.value(id);
        for (const auto &dependency : pass.dependsOn) {
            const auto dependencyResult = visit(dependency);
            if (!dependencyResult.ok) return dependencyResult;
        }
        marks.insert(id, Mark::Complete);
        order.append(id);
        return {true, 0, {}, {}};
    };

    for (const auto &pass : passes_) {
        const auto result = visit(pass.id);
        if (!result.ok) return result;
    }
    return {true, 0, order, {}};
}

RenderGraphResult RenderGraph::run(bool outputAvailable, const std::function<bool(const RenderPass &)> &execute) const
{
    if (!outputAvailable) {
        return {false, static_cast<int>(astra::common::ErrorCode::ProjectionOutputDisconnected), {},
                QStringLiteral("Projection output is unavailable")};
    }
    const auto ordered = topologicalOrder();
    if (!ordered.ok) return ordered;

    QHash<QString, RenderPass> passById;
    for (const auto &pass : passes_) passById.insert(pass.id, pass);
    for (const auto &id : ordered.order) {
        if (!execute(passById.value(id))) {
            return {false, static_cast<int>(astra::common::ErrorCode::ProjectionRenderPassFailed), ordered.order,
                    QStringLiteral("Projection render pass failed")};
        }
    }
    return ordered;
}

RenderGraph RenderGraph::fixedP4Pipeline()
{
    return RenderGraph {{
        {QStringLiteral("InputPass"), {}},
        {QStringLiteral("ScenePass"), {QStringLiteral("InputPass")}},
        {QStringLiteral("LayerCompositionPass"), {QStringLiteral("ScenePass")}},
        {QStringLiteral("CropAndOverscanPass"), {QStringLiteral("LayerCompositionPass")}},
        {QStringLiteral("GeometryWarpPass"), {QStringLiteral("CropAndOverscanPass")}},
        {QStringLiteral("ColorCompensationPass"), {QStringLiteral("GeometryWarpPass")}},
        {QStringLiteral("PrivacyMaskPass"), {QStringLiteral("ColorCompensationPass")}},
        {QStringLiteral("OutputPass"), {QStringLiteral("PrivacyMaskPass")}},
    }};
}

} // namespace astra::render
