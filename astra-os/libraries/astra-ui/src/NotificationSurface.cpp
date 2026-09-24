#include "astra/ui/NotificationSurface.h"

namespace astra::ui {

SpatialNotification::SpatialNotification(NotificationSpec spec)
    : spec_(std::move(spec))
{
}

QString SpatialNotification::notificationId() const { return spec_.notificationId; }
NotificationSeverity SpatialNotification::severity() const { return spec_.severity; }
bool SpatialNotification::requestsSystemFocus() const { return spec_.severity == NotificationSeverity::Critical; }

bool SpatialNotification::expiredAt(const QDateTime &now) const
{
    if (spec_.expiresAt.isValid()) return spec_.expiresAt <= now;
    return spec_.timeoutMs > 0 && spec_.createdAt.addMSecs(spec_.timeoutMs) <= now;
}

const NotificationSpec &SpatialNotification::spec() const { return spec_; }

OperationResult NotificationSurfaceManager::create(const NotificationSpec &spec)
{
    if (spec.notificationId.isEmpty() || spec.title.isEmpty() || spec.message.isEmpty() || !spec.createdAt.isValid()
        || spec.timeoutMs < 0 || (spec.expiresAt.isValid() && spec.expiresAt < spec.createdAt)
        || notifications_.contains(spec.notificationId)) {
        return {false, 5103, QStringLiteral("Invalid notification")};
    }
    if (spec.requiresAction && spec.actions.isEmpty() && spec.severity != NotificationSeverity::Critical) {
        return {false, 5402, QStringLiteral("Actionable notification requires an action")};
    }
    notifications_.insert(spec.notificationId, std::make_shared<SpatialNotification>(spec));
    return {true, 0, {}};
}

OperationResult NotificationSurfaceManager::remove(const QString &notificationId)
{
    if (!notifications_.contains(notificationId)) return {false, 5105, QStringLiteral("Notification not found")};
    notifications_.remove(notificationId);
    return {true, 0, {}};
}

SpatialNotification *NotificationSurfaceManager::find(const QString &notificationId) const
{
    return notifications_.value(notificationId).get();
}

qsizetype NotificationSurfaceManager::expire(const QDateTime &now)
{
    qsizetype removed = 0;
    for (auto iterator = notifications_.begin(); iterator != notifications_.end();) {
        if (iterator.value()->expiredAt(now)) {
            iterator = notifications_.erase(iterator);
            ++removed;
        } else {
            ++iterator;
        }
    }
    return removed;
}

void NotificationSurfaceManager::clear() { notifications_.clear(); }

qsizetype NotificationSurfaceManager::size() const { return notifications_.size(); }

} // namespace astra::ui
