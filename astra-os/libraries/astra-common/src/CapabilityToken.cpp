#include "astra/common/CapabilityToken.h"
#include "astra/common/MethodCapabilityTable.h"

#include <QCryptographicHash>

namespace astra::common {

QString scopedCapabilityToken(const QString &baseToken, const QString &capability)
{
    return QString::fromLatin1(QCryptographicHash::hash((baseToken + QLatin1Char(':') + capability).toUtf8(),
                                                       QCryptographicHash::Sha256)
                                   .toHex());
}

QString projectionCapabilityForMethod(const QString &method)
{
    return method == QStringLiteral("system.health") || method.startsWith(QStringLiteral("projection."))
        ? generated::methodCapability(method) : QString {};
}

QString spatialUICapabilityForMethod(const QString &method)
{
    return method == QStringLiteral("system.health") || method.startsWith(QStringLiteral("spatial_ui."))
        ? generated::methodCapability(method) : QString {};
}

} // namespace astra::common
