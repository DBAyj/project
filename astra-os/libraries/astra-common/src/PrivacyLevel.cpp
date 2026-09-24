#include "astra/common/PrivacyLevel.h"

namespace astra::common {
std::string_view privacyLevelToString(PrivacyLevel level)
{
    switch (level) {
    case PrivacyLevel::Public: return "PUBLIC";
    case PrivacyLevel::RoomOnly: return "ROOM_ONLY";
    case PrivacyLevel::AuthorizedPerson: return "AUTHORIZED_PERSON";
    case PrivacyLevel::PrivateScreenOnly: return "PRIVATE_SCREEN_ONLY";
    case PrivacyLevel::NoProjection: return "NO_PROJECTION";
    }
    return "NO_PROJECTION";
}
std::optional<PrivacyLevel> privacyLevelFromString(std::string_view value)
{
    for (auto level : {PrivacyLevel::Public, PrivacyLevel::RoomOnly, PrivacyLevel::AuthorizedPerson, PrivacyLevel::PrivateScreenOnly, PrivacyLevel::NoProjection}) {
        if (privacyLevelToString(level) == value) return level;
    }
    return std::nullopt;
}
} // namespace astra::common
