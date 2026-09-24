#include "astra/ui/TaskSurface.h"

#include "astra/common/PrivacyLevel.h"

#include <cmath>

namespace astra::ui {
namespace {

bool allowed(TaskSurfaceState from, TaskSurfaceState to)
{
    using State = TaskSurfaceState;
    switch (from) {
    case State::Created: return to == State::WaitingConfirmation || to == State::Ready || to == State::Cancelled;
    case State::WaitingConfirmation: return to == State::Ready || to == State::Cancelled;
    case State::Ready: return to == State::Running || to == State::Cancelled;
    case State::Running: return to == State::Paused || to == State::Completed || to == State::Failed || to == State::Cancelled;
    case State::Paused: return to == State::Running || to == State::Cancelled;
    case State::Completed:
    case State::Failed:
    case State::Cancelled: return false;
    }
    return false;
}

} // namespace

TaskSurface::TaskSurface(TaskSurfaceSpec spec)
    : spec_(std::move(spec))
{
}

QString TaskSurface::taskId() const { return spec_.taskId; }
QString TaskSurface::title() const { return spec_.title; }
QString TaskSurface::summary() const { return spec_.summary; }
QString TaskSurface::intentType() const { return spec_.intentType; }
double TaskSurface::confidence() const { return spec_.confidence; }
QString TaskSurface::executionStrategy() const { return spec_.executionStrategy; }
TaskSurfaceState TaskSurface::state() const { return state_; }
double TaskSurface::progress() const { return progress_; }
astra::common::PrivacyLevel TaskSurface::privacyLevel() const { return spec_.privacyLevel; }

OperationResult TaskSurface::transitionTo(TaskSurfaceState next)
{
    if (!allowed(state_, next)) return {false, 5601, QStringLiteral("Invalid task surface transition")};
    state_ = next;
    if (state_ == TaskSurfaceState::Completed) progress_ = 1.0;
    return {true, 0, {}};
}

OperationResult TaskSurface::setProgress(double progress)
{
    if (!std::isfinite(progress) || progress < 0.0 || progress > 1.0) return {false, 5402, QStringLiteral("Invalid task progress")};
    progress_ = progress;
    return {true, 0, {}};
}

TaskSurfaceCreateResult TaskSurfaceManager::createFromIntentFixture(const QJsonObject &fixture)
{
    const QString id = fixture.value(QStringLiteral("task_id")).toString();
    if (id.isEmpty() || surfaces_.contains(id)) return {{false, 5103, QStringLiteral("Invalid task id")}, nullptr};
    const auto privacy = astra::common::privacyLevelFromString(fixture.value(QStringLiteral("privacy_level")).toString().toStdString());
    const double confidence = fixture.value(QStringLiteral("confidence")).toDouble(-1.0);
    if (!privacy || confidence < 0.0 || confidence > 1.0 || fixture.value(QStringLiteral("title")).toString().isEmpty()
        || fixture.value(QStringLiteral("summary")).toString().isEmpty() || fixture.value(QStringLiteral("intent_type")).toString().isEmpty()) {
        return {{false, 5103, QStringLiteral("Invalid intent fixture")}, nullptr};
    }
    TaskSurfaceSpec spec {id, fixture.value(QStringLiteral("title")).toString(), fixture.value(QStringLiteral("summary")).toString(),
                          fixture.value(QStringLiteral("intent_type")).toString(), confidence,
                          fixture.value(QStringLiteral("execution_strategy")).toString(), *privacy};
    auto surface = std::make_shared<TaskSurface>(std::move(spec));
    auto *raw = surface.get();
    surfaces_.insert(id, std::move(surface));
    return {{true, 0, {}}, raw};
}

TaskSurfaceCreateResult TaskSurfaceManager::updateFromIntentFixture(const QJsonObject &fixture)
{
    const QString id = fixture.value(QStringLiteral("task_id")).toString();
    auto *surface = find(id);
    if (!surface || id.isEmpty()) return {{false, 5105, QStringLiteral("Task surface not found")}, nullptr};
    if (fixture.contains(QStringLiteral("progress"))) {
        const auto progress = fixture.value(QStringLiteral("progress"));
        if (!progress.isDouble()) return {{false, 5103, QStringLiteral("Invalid task progress")}, nullptr};
        const auto result = surface->setProgress(progress.toDouble());
        if (!result.ok) return {result, nullptr};
    }
    if (fixture.contains(QStringLiteral("state"))) {
        const auto state = fixture.value(QStringLiteral("state")).toString();
        const QHash<QString, TaskSurfaceState> states {
            {QStringLiteral("WAITING_CONFIRMATION"), TaskSurfaceState::WaitingConfirmation},
            {QStringLiteral("READY"), TaskSurfaceState::Ready}, {QStringLiteral("RUNNING"), TaskSurfaceState::Running},
            {QStringLiteral("PAUSED"), TaskSurfaceState::Paused}, {QStringLiteral("COMPLETED"), TaskSurfaceState::Completed},
            {QStringLiteral("FAILED"), TaskSurfaceState::Failed}, {QStringLiteral("CANCELLED"), TaskSurfaceState::Cancelled}};
        if (!states.contains(state)) return {{false, 5103, QStringLiteral("Invalid task state")}, nullptr};
        const auto result = surface->transitionTo(states.value(state));
        if (!result.ok) return {result, nullptr};
    }
    return {{true, 0, {}}, surface};
}

OperationResult TaskSurfaceManager::remove(const QString &taskId)
{
    if (!surfaces_.contains(taskId)) return {false, 5105, QStringLiteral("Task surface not found")};
    surfaces_.remove(taskId);
    return {true, 0, {}};
}

void TaskSurfaceManager::clear() { surfaces_.clear(); }

TaskSurface *TaskSurfaceManager::find(const QString &taskId) const { return surfaces_.value(taskId).get(); }
qsizetype TaskSurfaceManager::size() const { return surfaces_.size(); }

} // namespace astra::ui
