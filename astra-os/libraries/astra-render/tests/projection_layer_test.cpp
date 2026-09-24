#include "astra/render/ProjectionLayer.h"

#include "astra/common/PrivacyLevel.h"

#include <cassert>

int main()
{
    using astra::render::LayerType;
    using astra::render::ProjectionLayer;

    const auto ordered = astra::render::compositionOrder({
        {"debug", LayerType::DebugOverlay, 6, true, astra::common::PrivacyLevel::Public, {}, {}},
        {"privacy", LayerType::PrivacyMask, 0, true, astra::common::PrivacyLevel::Public, {}, {}},
        {"scene", LayerType::Scene3D, 1, true, astra::common::PrivacyLevel::Public, {}, {}},
        {"background", LayerType::Background, 0, true, astra::common::PrivacyLevel::Public, {}, {}},
    });

    assert(ordered.size() == 4);
    assert(ordered.at(0).id == "background");
    assert(ordered.at(1).id == "scene");
    assert(ordered.at(2).id == "debug");
    assert(ordered.at(3).id == "privacy");
    assert(ordered.at(1).privacyLevel == astra::common::PrivacyLevel::Public);
}
