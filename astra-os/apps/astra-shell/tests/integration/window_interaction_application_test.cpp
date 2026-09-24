#include "controllers/ShellController.h"

#include <QCoreApplication>
#include <QGuiApplication>
#include <QMetaObject>
#include <QQmlApplicationEngine>
#include <QQmlComponent>
#include <QQuickStyle>
#include <QQuickItem>
#include <QQuickWindow>
#include <QTemporaryDir>
#include <QUrl>
#include <QWheelEvent>
#include <QtTest/QTest>

#include <cassert>

namespace {

QObject *createWindow(QQmlApplicationEngine &engine, const QString &path, astra::shell::ShellController &controller)
{
    QQmlComponent component(&engine, QUrl::fromLocalFile(path));
    assert(component.isReady());
    QObject *root = component.createWithInitialProperties(
        {{QStringLiteral("controller"), QVariant::fromValue(static_cast<QObject *>(&controller))}});
    assert(root != nullptr);
    root->setParent(&engine);
    return root;
}

void click(QObject *button)
{
    assert(button != nullptr);
    assert(QMetaObject::invokeMethod(button, "click"));
    QCoreApplication::processEvents();
}

} // namespace

int main(int argc, char *argv[])
{
    QQuickStyle::setStyle(QStringLiteral("Basic"));
    QGuiApplication application(argc, argv);
    QTemporaryDir directory;
    astra::shell::SimulatorConfig config;
    astra::shell::ShellController controller(config, directory.filePath("audit/audit.jsonl"));
    QQmlApplicationEngine engine;

    const QString qmlRoot = QStringLiteral(ASTRA_QML_SOURCE_DIR);
    auto *phone = qobject_cast<QQuickWindow *>(createWindow(engine, qmlRoot + "/phone/PhoneWindow.qml", controller));
    auto *projection = qobject_cast<QQuickWindow *>(createWindow(engine, qmlRoot + "/projection/ProjectionWindow.qml", controller));
    assert(phone != nullptr);
    assert(projection != nullptr);
    phone->show();
    projection->show();
    QCoreApplication::processEvents();
    assert(phone->isVisible());
    assert(projection->isVisible());
    assert(phone->title() == QStringLiteral("AstraOS Phone Display"));
    assert(projection->title() == QStringLiteral("AstraOS Projection Display"));

    auto *input = phone->findChild<QObject *>(QStringLiteral("taskInput"));
    auto *execute = phone->findChild<QObject *>(QStringLiteral("executeButton"));
    input->setProperty("text", QStringLiteral("把设备模型投到桌面上"));
    click(execute);
    assert(controller.projectionState() == QStringLiteral("ACTIVE"));
    assert(controller.projectionModel()->modelVisible());

    click(projection->findChild<QObject *>(QStringLiteral("pauseRotationButton")));
    assert(controller.projectionState() == QStringLiteral("PAUSED"));
    click(projection->findChild<QObject *>(QStringLiteral("pauseRotationButton")));
    assert(controller.projectionState() == QStringLiteral("ACTIVE"));
    auto *scene = projection->findChild<QQuickItem *>(QStringLiteral("projectionScene"));
    assert(scene != nullptr);
    assert(scene->isVisible());
    const QPoint scenePoint = scene->mapToScene(QPointF(scene->width() / 2.0, scene->height() / 2.0)).toPoint();
    const double rotationBeforeDrag = controller.projectionModel()->sceneRotation();
    QTest::mousePress(projection, Qt::LeftButton, Qt::NoModifier, scenePoint);
    QTest::mouseMove(projection, scenePoint + QPoint(80, 0));
    QTest::mouseRelease(projection, Qt::LeftButton, Qt::NoModifier, scenePoint + QPoint(80, 0));
    QCoreApplication::processEvents();
    assert(controller.projectionModel()->sceneRotation() != rotationBeforeDrag);
    const double zoomBeforeWheel = controller.projectionModel()->zoom();
    QWheelEvent wheel(scenePoint,
                      projection->mapToGlobal(scenePoint),
                      QPoint(),
                      QPoint(0, 120),
                      Qt::NoButton,
                      Qt::NoModifier,
                      Qt::ScrollUpdate,
                      false);
    QCoreApplication::sendEvent(projection, &wheel);
    QCoreApplication::processEvents();
    assert(controller.projectionModel()->zoom() > zoomBeforeWheel);
    click(projection->findChild<QObject *>(QStringLiteral("resetViewButton")));
    assert(controller.projectionModel()->sceneRotation() == 0.0);
    assert(controller.projectionModel()->zoom() == 1.0);
    click(projection->findChild<QObject *>(QStringLiteral("fullscreenButton")));
    assert(controller.projectionFullscreen());
    assert(projection->visibility() == QWindow::FullScreen);
    click(projection->findChild<QObject *>(QStringLiteral("fullscreenButton")));
    assert(!controller.projectionFullscreen());
    assert(projection->visibility() == QWindow::Windowed);
    click(projection->findChild<QObject *>(QStringLiteral("stopProjectionButton")));
    assert(controller.projectionState() == QStringLiteral("IDLE"));
    assert(!controller.projectionModel()->modelVisible());

    auto *privacy = phone->findChild<QObject *>(QStringLiteral("privacySelector"));
    privacy->setProperty("currentIndex", 4);
    input->setProperty("text", QStringLiteral("开始投影"));
    click(execute);
    assert(controller.projectionState() == QStringLiteral("IDLE"));
    assert(!controller.projectionModel()->modelVisible());
    assert(controller.lastErrorCode() == 4301);
}
