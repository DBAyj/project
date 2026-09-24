#include "application/AstraApplication.h"

#include <QGuiApplication>
#include <QQuickStyle>

int main(int argc, char *argv[])
{
    QQuickStyle::setStyle(QStringLiteral("Basic"));
    QGuiApplication application(argc, argv);
    astra::shell::AstraApplication astraApplication(application);
    if (!astraApplication.initialize()) return 1;
    return application.exec();
}
