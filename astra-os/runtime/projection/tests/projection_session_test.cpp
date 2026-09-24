#include "astra/projection/ProjectionSession.h"

#include <cassert>

int main()
{
    using astra::projection::ProjectionSession;
    using astra::projection::SessionState;

    ProjectionSession session {"session-001"};
    assert(session.state() == SessionState::Idle);
    const auto invalidPause = session.pause();
    assert(!invalidPause.ok);
    assert(invalidPause.errorCode == 4004);
    assert(session.auditEvents().size() == 1);

    assert(session.initialize().ok);
    assert(session.ready().ok);
    assert(session.startRendering().ok);
    assert(session.pause().ok);
    assert(session.resume().ok);
    assert(session.beginClear().ok);
    assert(session.clearComplete().ok);
    assert(session.state() == SessionState::Ready);
    assert(session.beginStop().ok);
    assert(session.stopComplete().ok);
    assert(session.state() == SessionState::Stopped);
    assert(session.reset().ok);
    assert(session.state() == SessionState::Idle);
}
