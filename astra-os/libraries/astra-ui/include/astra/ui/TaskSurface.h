#pragma once

#include "astra/ui/Types.h"

#include <QHash>
#include <QJsonObject>

#include <memory>

namespace astra::ui {

enum class TaskSurfaceState { Created, WaitingConfirmation, Ready, Running, Paused, Completed, Failed, Cancelled };

struct TaskSurfaceSpec {
    QString taskId;
    QString title;
    QString summary;
    QString intentType;
    double confidence {0.0};
    QString executionStrategy;
    astra::common::PrivacyLevel privacyLevel {astra::common::PrivacyLevel::PrivateScreenOnly};
};

class TaskSurface final {
public:
    explicit TaskSurface(TaskSurfaceSpec spec);
    [[nodiscard]] QString taskId() const;
    [[nodiscard]] QString title() const;
    [[nodiscard]] QString summary() const;
    [[nodiscard]] QString intentType() const;
    [[nodiscard]] double confidence() const;
    [[nodiscard]] QString executionStrategy() const;
    [[nodiscard]] TaskSurfaceState state() const;
    [[nodiscard]] double progress() const;
    [[nodiscard]] astra::common::PrivacyLevel privacyLevel() const;
    OperationResult transitionTo(TaskSurfaceState next);
    OperationResult setProgress(double progress);

private:
    TaskSurfaceSpec spec_;
    TaskSurfaceState state_ {TaskSurfaceState::Created};
    double progress_ {0.0};
};

struct TaskSurfaceCreateResult : OperationResult {
    TaskSurface *surface {nullptr};
};

class TaskSurfaceManager final {
public:
    TaskSurfaceCreateResult createFromIntentFixture(const QJsonObject &fixture);
    TaskSurfaceCreateResult updateFromIntentFixture(const QJsonObject &fixture);
    OperationResult remove(const QString &taskId);
    void clear();
    [[nodiscard]] TaskSurface *find(const QString &taskId) const;
    [[nodiscard]] qsizetype size() const;

private:
    QHash<QString, std::shared_ptr<TaskSurface>> surfaces_;
};

} // namespace astra::ui
