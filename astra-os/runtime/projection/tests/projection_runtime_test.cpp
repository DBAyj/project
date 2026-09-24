#include "astra/projection/ProjectionRuntime.h"

#include <QColor>
#include <QImage>

#include <cassert>

int main()
{
    using astra::common::PrivacyLevel;
    using astra::projection::ProjectionRenderRequest;
    using astra::projection::ProjectionRuntime;
    using astra::projection::ProjectionSession;
    using astra::render::LayerType;
    using astra::render::Point;
    using astra::render::ProjectionLayer;
    using astra::render::Resolution;
    using astra::render::WindowProjectionOutput;

    const Resolution resolution {10, 10, 60.0};
    WindowProjectionOutput output {"window-projection"};
    assert(output.initialize(resolution).ok);
    ProjectionSession session {"session-001"};
    assert(session.initialize().ok);
    assert(session.ready().ok);
    ProjectionRuntime runtime {output, session};

    QImage input {10, 10, QImage::Format_RGBA8888};
    input.fill(Qt::red);
    ProjectionRenderRequest request;
    request.input = input;
    request.privacyLevel = PrivacyLevel::Public;
    request.layers = {{"scene", LayerType::Scene3D, 0, true, PrivacyLevel::Public, {}, {}}};
    request.corners = {Point {0.0, 0.0}, Point {9.0, 0.0}, Point {9.0, 9.0}, Point {0.0, 9.0}};
    assert(runtime.render(request).ok);
    assert(output.frame().pixelColor(0, 0) == QColor(Qt::red));

    request.layers = {{"private", LayerType::ApplicationSurface, 0, true, PrivacyLevel::PrivateScreenOnly, {}, {}}};
    const auto privateLayer = runtime.render(request);
    assert(!privateLayer.ok);
    assert(output.frame().pixelColor(0, 0) == QColor(Qt::black));
    assert(session.state() == astra::projection::SessionState::Stopped);

    assert(session.reset().ok);
    assert(output.initialize(resolution).ok);
    assert(session.initialize().ok);
    assert(session.ready().ok);
    request.layers = {{"scene", LayerType::Scene3D, 0, true, PrivacyLevel::Public, {}, {}}};

    input.fill(QColor {64, 64, 64});
    request.input = input;
    request.colorCompensation.gamma = 2.0;
    assert(runtime.render(request).ok);
    assert(output.frame().pixelColor(5, 5).red() == 128);

    request.colorCompensation = {};
    request.meshWarp = astra::render::MeshWarpSettings {2, 2, {{0.0, 0.0}, {0.5, 0.0}, {0.0, 1.0}, {0.5, 1.0}}};
    assert(runtime.render(request).ok);
    assert(output.frame().pixelColor(5, 5) == QColor(Qt::black));

    request.meshWarp = astra::render::MeshWarpSettings {2, 2, {{0.0, 0.0}, {1.0, 0.0}, {1.0, 1.0}, {0.0, 1.0}}};
    const auto invalidMesh = runtime.render(request);
    assert(!invalidMesh.ok);
    assert(output.frame().pixelColor(0, 0) == QColor(Qt::black));

    request.privacyLevel = PrivacyLevel::NoProjection;
    assert(runtime.render(request).ok);
    assert(output.frame().pixelColor(0, 0) == QColor(Qt::black));
    assert(session.state() == astra::projection::SessionState::Stopped);
}
