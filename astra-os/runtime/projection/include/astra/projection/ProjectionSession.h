#pragma once

#include <QString>
#include <QVector>

namespace astra::projection {

enum class SessionState { Idle, Initializing, Ready, Rendering, Paused, Clearing, Stopping, Stopped, Error };

struct SessionTransitionResult {
    bool ok {false};
    int errorCode {0};
    SessionState state {SessionState::Idle};
};

struct SessionAuditEvent {
    QString action;
    SessionState from {SessionState::Idle};
    SessionState to {SessionState::Idle};
    int errorCode {0};
};

class ProjectionSession final {
public:
    explicit ProjectionSession(QString sessionId);

    SessionTransitionResult initialize();
    SessionTransitionResult ready();
    SessionTransitionResult startRendering();
    SessionTransitionResult pause();
    SessionTransitionResult resume();
    SessionTransitionResult beginClear();
    SessionTransitionResult clearComplete();
    SessionTransitionResult beginStop();
    SessionTransitionResult stopComplete();
    SessionTransitionResult fail(int errorCode);
    SessionTransitionResult reset();

    QString sessionId() const;
    SessionState state() const;
    QVector<SessionAuditEvent> auditEvents() const;

private:
    SessionTransitionResult transition(const QString &action, SessionState target);
    bool allowed(SessionState target) const;

    QString sessionId_;
    SessionState state_ {SessionState::Idle};
    QVector<SessionAuditEvent> auditEvents_;
};

} // namespace astra::projection
