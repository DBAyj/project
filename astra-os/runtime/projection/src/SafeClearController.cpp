#include "astra/projection/SafeClearController.h"

#include "astra/common/ErrorCode.h"

namespace astra::projection {
namespace {

SafeClearResult failed(ProjectionSession &session)
{
    const int errorCode = static_cast<int>(astra::common::ErrorCode::ProjectionSafeClearFailed);
    session.fail(errorCode);
    return {false, errorCode};
}

} // namespace

SafeClearResult SafeClearController::clear(astra::render::ProjectionOutput &output, ProjectionSession &session, SafeClearReason reason)
{
    static_cast<void>(reason);
    const auto clearTransition = session.beginClear();
    const auto clearResult = output.clear();
    if (!clearResult.ok) return failed(session);
    if (!clearTransition.ok && session.state() != SessionState::Idle && session.state() != SessionState::Stopped) return failed(session);
    if (clearTransition.ok && !session.clearComplete().ok) return failed(session);
    if (session.state() == SessionState::Ready) {
        if (!session.beginStop().ok || !session.stopComplete().ok) return failed(session);
    }
    return {true, 0};
}

} // namespace astra::projection
