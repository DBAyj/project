#include "astra/ui/TaskSurface.h"

#include <QJsonObject>
#include <QtGlobal>

using namespace astra::ui;

namespace {
void require(bool condition)
{
    if (!condition) qFatal("P5 task surface requirement failed");
}
}

int main()
{
    TaskSurfaceManager tasks;
    const QJsonObject fixture {{QStringLiteral("task_id"), QStringLiteral("task-1")},
                               {QStringLiteral("title"), QStringLiteral("Inspect device")},
                               {QStringLiteral("summary"), QStringLiteral("Fixture intent result")},
                               {QStringLiteral("intent_type"), QStringLiteral("open_task_surface")},
                               {QStringLiteral("confidence"), 0.92},
                               {QStringLiteral("execution_strategy"), QStringLiteral("confirm")},
                               {QStringLiteral("privacy_level"), QStringLiteral("PRIVATE_SCREEN_ONLY")}};
    const auto created = tasks.createFromIntentFixture(fixture);
    require(created.ok && created.surface != nullptr);
    require(created.surface->state() == TaskSurfaceState::Created);
    require(created.surface->transitionTo(TaskSurfaceState::WaitingConfirmation).ok);
    require(created.surface->transitionTo(TaskSurfaceState::Ready).ok);
    require(created.surface->transitionTo(TaskSurfaceState::Running).ok);
    require(created.surface->setProgress(0.5).ok);
    require(!created.surface->setProgress(1.5).ok);
    require(created.surface->transitionTo(TaskSurfaceState::Completed).ok);
    require(!created.surface->transitionTo(TaskSurfaceState::Running).ok);
    require(created.surface->state() == TaskSurfaceState::Completed);
    require(tasks.size() == 1);
    tasks.clear();
    require(tasks.size() == 0 && tasks.find(QStringLiteral("task-1")) == nullptr);
}
