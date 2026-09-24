#pragma once

#include <QString>

namespace astra::common {

// Derive a method-scoped capability without exposing the base credential to a
// service handler. The base token remains only the input to this derivation.
[[nodiscard]] QString scopedCapabilityToken(const QString &baseToken, const QString &capability);
[[nodiscard]] QString projectionCapabilityForMethod(const QString &method);
[[nodiscard]] QString spatialUICapabilityForMethod(const QString &method);

} // namespace astra::common
