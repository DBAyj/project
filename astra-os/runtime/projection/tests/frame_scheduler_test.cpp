#include "astra/projection/FrameScheduler.h"

#include <cassert>

int main()
{
    astra::projection::FrameScheduler scheduler {60.0};
    const auto initial = scheduler.schedule(0);
    assert(initial.render);
    assert(initial.targetFrameRateHz == 60.0);

    const auto early = scheduler.schedule(1'000'000);
    assert(!early.render);
    assert(early.dropped);

    scheduler.noteRenderDuration(20'000'000);
    const auto degraded = scheduler.schedule(34'000'000);
    assert(degraded.render);
    assert(degraded.targetFrameRateHz == 30.0);

    scheduler.pause();
    assert(!scheduler.schedule(100'000'000).render);
    scheduler.resume();
    assert(scheduler.schedule(101'000'000).render);
}
