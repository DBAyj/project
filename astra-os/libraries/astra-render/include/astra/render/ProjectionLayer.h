#pragma once

#include "astra/common/PrivacyLevel.h"

#include <QRectF>
#include <QString>
#include <QVector>

namespace astra::render {

enum class LayerType { Background, Scene3D, ApplicationSurface, AiAssistant, Notification, PrivacyMask, DebugOverlay };

struct ProjectionLayer {
    QString id;
    LayerType type {LayerType::Background};
    int zIndex {0};
    bool visible {true};
    astra::common::PrivacyLevel privacyLevel {astra::common::PrivacyLevel::NoProjection};
    QRectF bounds;
    QString publicLabel;
    QString policyDecisionId;
    QString policySubjectId;
};

QVector<ProjectionLayer> compositionOrder(QVector<ProjectionLayer> layers);

} // namespace astra::render
