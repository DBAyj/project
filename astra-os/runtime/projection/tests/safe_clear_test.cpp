#include "astra/projection/SafeClearController.h"

#include <QColor>
#include <QImage>

#include <cassert>

int main()
{
    using astra::projection::ProjectionSession;
    using astra::projection::SafeClearController;
    using astra::projection::SafeClearReason;
    using astra::projection::SessionState;
    using astra::render::Resolution;
    using astra::render::WindowProjectionOutput;

    WindowProjectionOutput output {"window-projection"};
    const Resolution resolution {4, 4, 60.0};
    assert(output.initialize(resolution).ok);
    QImage frame {4, 4, QImage::Format_RGBA8888};
    frame.fill(Qt::red);
    assert(output.present(frame).ok);

    ProjectionSession session {"session-001"};
    assert(session.initialize().ok);
    assert(session.ready().ok);
    assert(session.startRendering().ok);
    const auto cleared = SafeClearController::clear(output, session, SafeClearReason::NoProjection);
    assert(cleared.ok);
    assert(output.frame().pixelColor(0, 0) == QColor(Qt::black));
    assert(session.state() == SessionState::Stopped);

    WindowProjectionOutput unavailable {"unavailable"};
    ProjectionSession failedSession {"session-002"};
    assert(failedSession.initialize().ok);
    assert(failedSession.ready().ok);
    const auto failed = SafeClearController::clear(unavailable, failedSession, SafeClearReason::OutputDisconnected);
    assert(!failed.ok);
    assert(failed.errorCode == 4016);
    assert(failedSession.state() == SessionState::Error);
}
