#pragma once

namespace astra::projection {

struct FrameScheduleDecision {
    bool render {false};
    bool dropped {false};
    double targetFrameRateHz {0.0};
};

class FrameScheduler final {
public:
    explicit FrameScheduler(double targetFrameRateHz);

    FrameScheduleDecision schedule(long long nowNanoseconds);
    void noteRenderDuration(long long durationNanoseconds);
    void pause();
    void resume();

private:
    double targetFrameRateHz_;
    long long lastRenderedAtNanoseconds_ {-1};
    bool paused_ {false};
};

} // namespace astra::projection
