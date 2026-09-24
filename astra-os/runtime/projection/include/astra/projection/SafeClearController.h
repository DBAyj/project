#pragma once

#include "astra/projection/ProjectionSession.h"
#include "astra/render/ProjectionOutput.h"

namespace astra::projection {

enum class SafeClearReason { NoProjection, PrivateScreenOnly, TargetLost, HomographyFailed, RenderPassFailed, OutputDisconnected, ServiceException, ManualStop };

struct SafeClearResult {
    bool ok {false};
    int errorCode {0};
};

class SafeClearController final {
public:
    static SafeClearResult clear(astra::render::ProjectionOutput &output, ProjectionSession &session, SafeClearReason reason);
};

} // namespace astra::projection
