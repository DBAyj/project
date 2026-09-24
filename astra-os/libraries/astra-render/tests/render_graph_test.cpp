#include "astra/render/RenderGraph.h"

#include <cassert>

int main()
{
    using astra::render::RenderGraph;
    using astra::render::RenderPass;

    RenderGraph graph {{
        {"OutputPass", {"ScenePass"}},
        {"ScenePass", {"InputPass"}},
        {"InputPass", {}},
    }};
    const auto ordered = graph.topologicalOrder();
    assert(ordered.ok);
    const QStringList expectedOrder {"InputPass", "ScenePass", "OutputPass"};
    assert(ordered.order == expectedOrder);

    RenderGraph missing {{ {"OutputPass", {"UnknownPass"}} }};
    assert(!missing.topologicalOrder().ok);
    assert(missing.topologicalOrder().errorCode == 4008);

    RenderGraph cycle {{ {"InputPass", {"OutputPass"}}, {"OutputPass", {"InputPass"}} }};
    assert(!cycle.topologicalOrder().ok);
    assert(cycle.topologicalOrder().errorCode == 4009);

    const auto failedPass = graph.run(true, [](const RenderPass &pass) { return pass.id != "ScenePass"; });
    assert(!failedPass.ok);
    assert(failedPass.errorCode == 4010);
    assert(!graph.run(false, [](const RenderPass &) { return true; }).ok);
    assert(graph.run(false, [](const RenderPass &) { return true; }).errorCode == 4007);
}
