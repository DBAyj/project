#include "astra/render/ProjectionLayer.h"

#include <algorithm>
#include <limits>

namespace astra::render {
namespace {

int compositionRank(const ProjectionLayer &layer)
{
    return layer.type == LayerType::PrivacyMask ? std::numeric_limits<int>::max() : layer.zIndex;
}

} // namespace

QVector<ProjectionLayer> compositionOrder(QVector<ProjectionLayer> layers)
{
    std::stable_sort(layers.begin(), layers.end(), [](const ProjectionLayer &left, const ProjectionLayer &right) {
        return compositionRank(left) < compositionRank(right);
    });
    return layers;
}

} // namespace astra::render
