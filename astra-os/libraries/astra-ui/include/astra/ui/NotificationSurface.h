#pragma once

#include "astra/ui/Types.h"

#include <QDateTime>
#include <QHash>
#include <QStringList>

#include <memory>

namespace astra::ui {

enum class NotificationSeverity { Info, Success, Warning, Error, Critical };

struct NotificationSpec {
    QString notificationId;
    QString title;
    QString message;
    NotificationSeverity severity {NotificationSeverity::Info};
    astra::common::PrivacyLevel privacyLevel {astra::common::PrivacyLevel::PrivateScreenOnly};
    DisplayTarget displayTarget {DisplayTarget::Phone};
    int timeoutMs {3000};
    bool requiresAction {false};
    QStringList actions;
    QDateTime createdAt;
    QDateTime expiresAt;
};

class SpatialNotification final {
public:
    explicit SpatialNotification(NotificationSpec spec);
    [[nodiscard]] QString notificationId() const;
    [[nodiscard]] NotificationSeverity severity() const;
    [[nodiscard]] bool requestsSystemFocus() const;
    [[nodiscard]] bool expiredAt(const QDateTime &now) const;
    [[nodiscard]] const NotificationSpec &spec() const;

private:
    NotificationSpec spec_;
};

class NotificationSurfaceManager final {
public:
    OperationResult create(const NotificationSpec &spec);
    OperationResult remove(const QString &notificationId);
    [[nodiscard]] SpatialNotification *find(const QString &notificationId) const;
    qsizetype expire(const QDateTime &now);
    void clear();
    [[nodiscard]] qsizetype size() const;

private:
    QHash<QString, std::shared_ptr<SpatialNotification>> notifications_;
};

} // namespace astra::ui
