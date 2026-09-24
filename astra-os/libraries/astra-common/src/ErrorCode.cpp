#include "astra/common/ErrorCode.h"

#include "astra/common/ErrorCodeRegistry.h"

namespace astra::common {
QString errorMessage(ErrorCode code)
{
    return ErrorCodeRegistry::lookup(static_cast<int>(code)).messageEn;
}
} // namespace astra::common
