#include "astra/common/Identifiers.h"

#include <array>
#include <chrono>
#include <iomanip>
#include <random>
#include <sstream>

namespace astra::common {
std::string newUuid()
{
    std::array<unsigned char, 16> bytes {};
    std::random_device device;
    for (auto &byte : bytes) byte = static_cast<unsigned char>(device());
    bytes[6] = static_cast<unsigned char>((bytes[6] & 0x0fU) | 0x40U);
    bytes[8] = static_cast<unsigned char>((bytes[8] & 0x3fU) | 0x80U);
    std::ostringstream stream;
    for (std::size_t index = 0; index < bytes.size(); ++index) {
        if (index == 4 || index == 6 || index == 8 || index == 10) stream << '-';
        stream << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(bytes[index]);
    }
    return stream.str();
}
std::string utcTimestamp()
{
    const auto now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    std::tm utc {};
    gmtime_r(&now, &utc);
    std::ostringstream stream;
    stream << std::put_time(&utc, "%Y-%m-%dT%H:%M:%SZ");
    return stream.str();
}
} // namespace astra::common
