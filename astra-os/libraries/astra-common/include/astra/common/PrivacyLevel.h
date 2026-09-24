#pragma once

#include <optional>
#include <string_view>

namespace astra::common {
enum class PrivacyLevel { Public, RoomOnly, AuthorizedPerson, PrivateScreenOnly, NoProjection };
std::string_view privacyLevelToString(PrivacyLevel level);
std::optional<PrivacyLevel> privacyLevelFromString(std::string_view value);
} // namespace astra::common
