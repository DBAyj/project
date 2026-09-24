#include "services/ProjectionSessionService.h"

#include <cassert>

int main()
{
    astra::shell::ProjectionSessionService service;
    assert(service.start().ok);
    assert(service.state() == "ACTIVE");
    assert(!service.sessionId().isEmpty());
    assert(service.start().errorCode == 4002);
    assert(service.pause().ok && service.state() == "PAUSED");
    assert(service.resume().ok && service.state() == "ACTIVE");
    assert(service.stop().ok && service.state() == "IDLE");
    assert(service.sessionId().isEmpty());
    assert(service.stop().errorCode == 4003);
    assert(service.pause().errorCode == 4004);
    assert(service.state() == "IDLE");
}
