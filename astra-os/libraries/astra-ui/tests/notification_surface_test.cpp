#include "astra/ui/NotificationSurface.h"

#include <QtGlobal>

using namespace astra::ui;

namespace {
void require(bool condition)
{
    if (!condition) qFatal("P5 notification requirement failed");
}
}

int main()
{
    const QDateTime now = QDateTime::fromString(QStringLiteral("2026-07-15T12:00:00Z"), Qt::ISODate);
    NotificationSurfaceManager manager;
    NotificationSpec warning {QStringLiteral("warning-1"), QStringLiteral("Target moving"), QStringLiteral("Recalibrating"),
                              NotificationSeverity::Warning, astra::common::PrivacyLevel::Public, DisplayTarget::Both, 3000, false, {}, now};
    require(manager.create(warning).ok);
    require(!manager.find(warning.notificationId)->requestsSystemFocus());
    require(manager.expire(now.addMSecs(2999)) == 0);
    require(manager.expire(now.addMSecs(3000)) == 1);
    require(manager.find(warning.notificationId) == nullptr);

    NotificationSpec critical = warning;
    critical.notificationId = QStringLiteral("critical-1");
    critical.severity = NotificationSeverity::Critical;
    critical.timeoutMs = 0;
    critical.requiresAction = true;
    require(manager.create(critical).ok);
    require(manager.find(critical.notificationId)->requestsSystemFocus());
    require(manager.expire(now.addDays(1)) == 0);

    NotificationSpec explicitExpiry = warning;
    explicitExpiry.notificationId = QStringLiteral("explicit-expiry");
    explicitExpiry.timeoutMs = 60'000;
    explicitExpiry.expiresAt = now.addMSecs(500);
    require(manager.create(explicitExpiry).ok);
    require(manager.expire(now.addMSecs(499)) == 0);
    require(manager.expire(now.addMSecs(500)) == 1);
    require(manager.find(explicitExpiry.notificationId) == nullptr);
}
