#include "astra/ui/SpatialFocusManager.h"

#include <QtGlobal>

using namespace astra::ui;

namespace {
void require(bool condition)
{
    if (!condition) qFatal("P5 spatial focus requirement failed");
}
}

int main()
{
    SpatialFocusManager focus;
    focus.registerTarget({QStringLiteral("editor"), QStringLiteral("workspace"), true, true, true, 2});
    focus.registerTarget({QStringLiteral("task"), QStringLiteral("workspace"), true, true, true, 1});
    focus.registerTarget({QStringLiteral("critical"), QStringLiteral("workspace"), true, true, true, 3});

    require(focus.requestFocus(QStringLiteral("editor"), FocusType::Keyboard, FocusPriority::ActiveWindow, QStringLiteral("typing")).ok);
    require(focus.owner(QStringLiteral("workspace")) == QStringLiteral("editor"));
    const auto denied = focus.requestFocus(QStringLiteral("task"), FocusType::Pointer, FocusPriority::NormalComponent, QStringLiteral("notification"));
    require(!denied.ok && denied.errorCode == 5503);
    require(focus.owner(QStringLiteral("workspace")) == QStringLiteral("editor"));

    require(focus.requestFocus(QStringLiteral("critical"), FocusType::System, FocusPriority::System, QStringLiteral("critical alert")).ok);
    require(focus.owner(QStringLiteral("workspace")) == QStringLiteral("critical"));
    require(focus.setAvailable(QStringLiteral("critical"), false).ok);
    require(focus.owner(QStringLiteral("workspace")).isEmpty());

    require(focus.focusNext(QStringLiteral("workspace")).ok);
    require(focus.owner(QStringLiteral("workspace")) == QStringLiteral("task"));
    require(focus.focusNext(QStringLiteral("workspace")).ok);
    require(focus.owner(QStringLiteral("workspace")) == QStringLiteral("editor"));
    require(focus.unregisterTarget(QStringLiteral("editor")).ok);
    require(focus.owner(QStringLiteral("workspace")).isEmpty());
}
