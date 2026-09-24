#include "application/AstraApplication.h"

#include <QGuiApplication>
#include <QQuickWindow>

#include <cassert>

int main(int argc, char *argv[])
{
    QGuiApplication application(argc, argv);
    assert(astra::shell::AstraApplication::resolveProjectRoot(QStringLiteral("/workspace/astra-os"), QStringLiteral("/fallback"))
           == QStringLiteral("/workspace/astra-os"));
    assert(astra::shell::AstraApplication::resolveProjectRoot(QString(), QStringLiteral("/fallback")) == QStringLiteral("/fallback"));
    assert(astra::shell::AstraApplication::configurationPath(QStringLiteral("/workspace/astra-os"))
           == QStringLiteral("/workspace/astra-os/config/astra.example.yaml"));
    assert(astra::shell::AstraApplication::runtimePath(QStringLiteral("/workspace/astra-os"), QStringLiteral("runtime/audit/audit.jsonl"))
           == QStringLiteral("/workspace/astra-os/runtime/audit/audit.jsonl"));

    QQuickWindow phone;
    phone.setTitle(QStringLiteral("AstraOS Phone Display"));
    QQuickWindow projection;
    projection.setTitle(QStringLiteral("AstraOS Projection Display"));
    const QList<QObject *> expectedRoots {&phone, &projection};
    assert(astra::shell::AstraApplication::hasExpectedRootWindows(expectedRoots));
    const QList<QObject *> incompleteRoots {&phone};
    assert(!astra::shell::AstraApplication::hasExpectedRootWindows(incompleteRoots));
}
