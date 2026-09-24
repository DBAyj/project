#include "astra/projection/ProjectionSession.h"

#include "astra/common/ErrorCode.h"

namespace astra::projection {

ProjectionSession::ProjectionSession(QString sessionId)
    : sessionId_(std::move(sessionId))
{
}

SessionTransitionResult ProjectionSession::initialize() { return transition(QStringLiteral("initialize"), SessionState::Initializing); }

SessionTransitionResult ProjectionSession::ready() { return transition(QStringLiteral("ready"), SessionState::Ready); }

SessionTransitionResult ProjectionSession::startRendering() { return transition(QStringLiteral("render"), SessionState::Rendering); }

SessionTransitionResult ProjectionSession::pause() { return transition(QStringLiteral("pause"), SessionState::Paused); }

SessionTransitionResult ProjectionSession::resume() { return transition(QStringLiteral("resume"), SessionState::Rendering); }

SessionTransitionResult ProjectionSession::beginClear() { return transition(QStringLiteral("clear"), SessionState::Clearing); }

SessionTransitionResult ProjectionSession::clearComplete() { return transition(QStringLiteral("clear_complete"), SessionState::Ready); }

SessionTransitionResult ProjectionSession::beginStop() { return transition(QStringLiteral("stop"), SessionState::Stopping); }

SessionTransitionResult ProjectionSession::stopComplete() { return transition(QStringLiteral("stop_complete"), SessionState::Stopped); }

SessionTransitionResult ProjectionSession::fail(int errorCode)
{
    if (state_ == SessionState::Stopped || state_ == SessionState::Idle) {
        auditEvents_.append({QStringLiteral("fail"), state_, SessionState::Error,
                             static_cast<int>(astra::common::ErrorCode::InvalidProjectionTransition)});
        return {false, static_cast<int>(astra::common::ErrorCode::InvalidProjectionTransition), state_};
    }
    const auto previous = state_;
    state_ = SessionState::Error;
    auditEvents_.append({QStringLiteral("fail"), previous, state_, errorCode});
    return {true, 0, state_};
}

SessionTransitionResult ProjectionSession::reset() { return transition(QStringLiteral("reset"), SessionState::Idle); }

QString ProjectionSession::sessionId() const { return sessionId_; }

SessionState ProjectionSession::state() const { return state_; }

QVector<SessionAuditEvent> ProjectionSession::auditEvents() const { return auditEvents_; }

SessionTransitionResult ProjectionSession::transition(const QString &action, SessionState target)
{
    if (!allowed(target)) {
        auditEvents_.append({action, state_, target, static_cast<int>(astra::common::ErrorCode::InvalidProjectionTransition)});
        return {false, static_cast<int>(astra::common::ErrorCode::InvalidProjectionTransition), state_};
    }
    const auto previous = state_;
    state_ = target;
    auditEvents_.append({action, previous, state_, 0});
    return {true, 0, state_};
}

bool ProjectionSession::allowed(SessionState target) const
{
    switch (state_) {
    case SessionState::Idle: return target == SessionState::Initializing;
    case SessionState::Initializing: return target == SessionState::Ready || target == SessionState::Stopping;
    case SessionState::Ready: return target == SessionState::Rendering || target == SessionState::Clearing || target == SessionState::Stopping;
    case SessionState::Rendering: return target == SessionState::Paused || target == SessionState::Clearing || target == SessionState::Stopping;
    case SessionState::Paused: return target == SessionState::Rendering || target == SessionState::Clearing || target == SessionState::Stopping;
    case SessionState::Clearing: return target == SessionState::Ready || target == SessionState::Stopping;
    case SessionState::Stopping: return target == SessionState::Stopped;
    case SessionState::Stopped: return target == SessionState::Idle;
    case SessionState::Error: return target == SessionState::Clearing || target == SessionState::Stopping;
    }
    return false;
}

} // namespace astra::projection
