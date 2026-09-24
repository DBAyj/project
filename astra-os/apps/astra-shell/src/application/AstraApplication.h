#pragma once

#include <QList>
#include <QString>

#include <memory>

class QGuiApplication;
class QQmlApplicationEngine;
class QObject;

namespace astra::shell {

class ShellController;

class AstraApplication final {
public:
    explicit AstraApplication(QGuiApplication &application);
    ~AstraApplication();

    AstraApplication(const AstraApplication &) = delete;
    AstraApplication &operator=(const AstraApplication &) = delete;

    bool initialize();

    static QString resolveProjectRoot(const QString &configuredRoot, const QString &fallbackDirectory);
    static QString configurationPath(const QString &projectRoot);
    static QString runtimePath(const QString &projectRoot, const QString &relativePath);
    static bool hasExpectedRootWindows(const QList<QObject *> &rootObjects);

private:
    QGuiApplication &application_;
    std::unique_ptr<ShellController> controller_;
    std::unique_ptr<QQmlApplicationEngine> engine_;
};

} // namespace astra::shell
