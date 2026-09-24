#include "astra/common/CapabilityToken.h"
#include "astra/common/ErrorCode.h"
#include "astra/common/Identifiers.h"
#include "astra/common/PrivacyLevel.h"

#include <cassert>
#include <regex>

int main()
{
    assert(astra::common::privacyLevelFromString("PUBLIC").value() == astra::common::PrivacyLevel::Public);
    assert(!astra::common::privacyLevelFromString("UNKNOWN").has_value());
    assert(astra::common::errorMessage(astra::common::ErrorCode::PrivacyPolicyDenied) == "Projection denied by privacy policy");
    assert(astra::common::errorMessage(static_cast<astra::common::ErrorCode>(9999)) == "Unclassified internal error");
    assert(std::regex_match(astra::common::newUuid(), std::regex("[0-9a-f]{8}-[0-9a-f]{4}-4[0-9a-f]{3}-[89ab][0-9a-f]{3}-[0-9a-f]{12}")));
    assert(astra::common::utcTimestamp().ends_with('Z'));
    assert(astra::common::spatialUICapabilityForMethod(QStringLiteral("system.health")) == QStringLiteral("system.health.read"));
    assert(astra::common::spatialUICapabilityForMethod(QStringLiteral("spatial_ui.target.lost")) == QStringLiteral("spatial_ui.system"));
    assert(astra::common::projectionCapabilityForMethod(QStringLiteral("spatial_ui.status")).isEmpty());
}
