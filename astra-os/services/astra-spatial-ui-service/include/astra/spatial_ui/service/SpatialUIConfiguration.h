#pragma once

#include "astra/spatial_ui/service/SpatialUIRuntime.h"

#include <QString>

namespace astra::spatial_ui::service {

struct SpatialUIConfigurationLoadResult {
    SpatialUIRuntimeOptions options;
    bool valid {false};
    bool schemaValidated {false};
    bool usingSafeDefaults {true};
    QString warning;
};

class SpatialUIConfiguration final {
public:
    [[nodiscard]] static SpatialUIConfigurationLoadResult load(const QString &configurationDirectory,
                                                               const QString &schemaDirectory);
};

} // namespace astra::spatial_ui::service
