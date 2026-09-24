#include "astra/ui/InputRouter.h"

#include <QtGlobal>

using namespace astra::ui;

namespace {
void require(bool condition)
{
    if (!condition) qFatal("P5 input routing requirement failed");
}
}

int main()
{
    InputRouter router;
    router.setTargets({
        {QStringLiteral("background"), {0.0, 0.0, 500.0, 400.0}, HitShape::Rectangle, {}, 0, false, false, true, true, 1.0},
        {QStringLiteral("card"), {100.0, 100.0, 200.0, 120.0}, HitShape::Rectangle, {}, 10, false, false, true, true, 1.0},
        {QStringLiteral("circle"), {150.0, 120.0, 80.0, 80.0}, HitShape::Circle, {}, 20, false, false, true, true, 1.0},
    });
    InputEvent event {QStringLiteral("event-1"), InputEventType::PointerPress, InputSourceType::Mouse,
                      QStringLiteral("primary-mouse"), CoordinateSystem::ProjectionView, 190.0, 160.0};
    auto routed = router.route(event);
    require(routed.ok && routed.targetComponentId == QStringLiteral("circle"));

    router.setTargets({
        {QStringLiteral("card"), {100.0, 100.0, 200.0, 120.0}, HitShape::Rectangle, {}, 100, false, false, true, true, 1.0},
        {QStringLiteral("privacy-confirmation"), {80.0, 80.0, 260.0, 180.0}, HitShape::Rectangle, {}, 1, true, true, true, true, 1.0},
    });
    routed = router.route(event);
    require(!routed.ok && routed.blocked);
    require(routed.errorCode == 5406);
    require(routed.targetComponentId == QStringLiteral("privacy-confirmation"));

    event.eventType = InputEventType::AiAction;
    event.targetComponentId = QStringLiteral("privacy-confirmation");
    routed = router.route(event);
    require(routed.ok && !routed.blocked);
    require(routed.targetComponentId == QStringLiteral("privacy-confirmation"));

    router.setTargets({
        {QStringLiteral("older-critical"), {80.0, 80.0, 260.0, 180.0}, HitShape::Rectangle, {}, 10001, true, true, true, true, 1.0},
        {QStringLiteral("newest-critical"), {90.0, 90.0, 260.0, 180.0}, HitShape::Rectangle, {}, 10002, true, true, true, true, 1.0},
    });
    event.targetComponentId = QStringLiteral("newest-critical");
    routed = router.route(event);
    require(routed.ok && !routed.blocked);
    require(routed.targetComponentId == QStringLiteral("newest-critical"));
    event.targetComponentId = QStringLiteral("older-critical");
    routed = router.route(event);
    require(!routed.ok && routed.blocked && routed.errorCode == 5406);
    require(routed.targetComponentId == QStringLiteral("newest-critical"));

    event.eventType = InputEventType::PointerPress;
    event.targetComponentId.clear();
    router.setTargets({{QStringLiteral("transparent"), {0.0, 0.0, 500.0, 400.0}, HitShape::Rectangle, {}, 100, false, false, true, true, 0.0}});
    routed = router.route(event);
    require(!routed.ok && routed.errorCode == 5404);

    event.eventType = InputEventType::AiAction;
    event.targetComponentId = QStringLiteral("disabled-target");
    router.setTargets({{QStringLiteral("disabled-target"), {0.0, 0.0, 500.0, 400.0}, HitShape::Rectangle, {},
                        100, false, false, true, true, 1.0, false}});
    routed = router.route(event);
    require(!routed.ok && routed.errorCode == 5403);

    event.eventType = InputEventType::PointerPress;
    event.targetComponentId.clear();
    router.setTargets({
        {QStringLiteral("enabled-background"), {0.0, 0.0, 500.0, 400.0}, HitShape::Rectangle, {},
         0, false, false, true, true, 1.0, true},
        {QStringLiteral("disabled-overlay"), {0.0, 0.0, 500.0, 400.0}, HitShape::Rectangle, {},
         100, false, false, true, true, 1.0, false},
    });
    routed = router.route(event);
    require(routed.ok && routed.targetComponentId == QStringLiteral("enabled-background"));

    router.setTargets({
        {QStringLiteral("parent"), {100.0, 100.0, 200.0, 120.0}, HitShape::Rectangle, {},
         10, false, false, true, true, 1.0, true, {}, 0},
        {QStringLiteral("child"), {100.0, 100.0, 200.0, 120.0}, HitShape::Rectangle, {},
         10, false, false, true, true, 1.0, true, QStringLiteral("parent"), 1},
    });
    routed = router.route(event);
    require(routed.ok && routed.targetComponentId == QStringLiteral("child"));

    event.x = 20.0;
    event.y = 20.0;
    event.targetComponentId = QStringLiteral("hinted-card");
    router.setTargets({
        {QStringLiteral("background"), {0.0, 0.0, 500.0, 400.0}, HitShape::Rectangle, {},
         0, false, false, true, true, 1.0, true},
        {QStringLiteral("hinted-card"), {100.0, 100.0, 200.0, 120.0}, HitShape::Rectangle, {},
         10, false, false, true, true, 1.0, true},
    });
    routed = router.route(event);
    require(routed.ok && routed.targetComponentId == QStringLiteral("background"));

    event.x = 700.0;
    event.y = 700.0;
    event.targetCaptured = true;
    routed = router.route(event);
    require(routed.ok && routed.targetComponentId == QStringLiteral("hinted-card"));
}
