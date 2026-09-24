#include "astra/projection/FrameScheduler.h"

#include <algorithm>

namespace astra::projection {
namespace {

long long frameIntervalNanoseconds(double framesPerSecond)
{
    return static_cast<long long>(1'000'000'000.0 / framesPerSecond);
}

} // namespace

FrameScheduler::FrameScheduler(double targetFrameRateHz)
    : targetFrameRateHz_(std::clamp(targetFrameRateHz, 1.0, 120.0))
{
}

FrameScheduleDecision FrameScheduler::schedule(long long nowNanoseconds)
{
    if (paused_) return {false, false, targetFrameRateHz_};
    if (lastRenderedAtNanoseconds_ >= 0
        && nowNanoseconds - lastRenderedAtNanoseconds_ < frameIntervalNanoseconds(targetFrameRateHz_)) {
        return {false, true, targetFrameRateHz_};
    }
    lastRenderedAtNanoseconds_ = nowNanoseconds;
    return {true, false, targetFrameRateHz_};
}

void FrameScheduler::noteRenderDuration(long long durationNanoseconds)
{
    if (durationNanoseconds > frameIntervalNanoseconds(targetFrameRateHz_)) targetFrameRateHz_ = std::max(15.0, targetFrameRateHz_ / 2.0);
}

void FrameScheduler::pause() { paused_ = true; }

void FrameScheduler::resume()
{
    paused_ = false;
    lastRenderedAtNanoseconds_ = -1;
}

} // namespace astra::projection
