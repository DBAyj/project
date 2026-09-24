#include "astra/projection/service/ProjectionService.h"
#include "clients/ProjectionServiceClient.h"

#include <QCoreApplication>
#include <QColor>
#include <QCryptographicHash>
#include <QDir>
#include <QFileInfo>
#include <QImage>
#include <QMetaObject>
#include <QUuid>

#include <cassert>
#include <thread>

int main(int argc, char *argv[])
{
    QCoreApplication application(argc, argv);
    const QString socketName = QDir::temp().filePath(QStringLiteral("astra-p4-%1.sock").arg(QUuid::createUuid().toString(QUuid::WithoutBraces)));
    const QString framePath = QDir::temp().filePath(QStringLiteral("astra-p4-%1.png").arg(QUuid::createUuid().toString(QUuid::WithoutBraces)));
    astra::projection::service::ProjectionService service;
    QString error;
    assert(service.listen(socketName, &error));

    bool passed = false;
    std::thread clientThread {[&] {
        astra::shell::ProjectionServiceClient client {socketName, {}, framePath};
        const auto started = client.start();
        const bool frameCaptured = QFileInfo::exists(framePath);
        QImage capturedFrame {framePath};
        capturedFrame = capturedFrame.convertToFormat(QImage::Format_RGBA8888);
        const QByteArray rawPixels {reinterpret_cast<const char *>(capturedFrame.constBits()), static_cast<qsizetype>(capturedFrame.sizeInBytes())};
        const bool frameVisualBaseline = frameCaptured && (capturedFrame.size() == QSize {256, 144})
            && capturedFrame.pixelColor(0, 0) == QColor {240, 240, 240} && capturedFrame.pixelColor(16, 0) == QColor {24, 24, 24}
            && QCryptographicHash::hash(rawPixels, QCryptographicHash::Sha256).toHex() == QByteArrayLiteral("7e8b5046a359e34fd2d0b005959761589c8b577157ebe0ccee33b3f6b88c0e79");
        const auto paused = client.pause();
        const auto resumed = client.resume();
        const auto stopped = client.stop();
        passed = started.ok && started.state == QStringLiteral("ACTIVE") && frameVisualBaseline && paused.ok && paused.state == QStringLiteral("PAUSED")
            && resumed.ok && resumed.state == QStringLiteral("ACTIVE") && stopped.ok && stopped.state == QStringLiteral("IDLE") && !QFileInfo::exists(framePath);
        QMetaObject::invokeMethod(&application, &QCoreApplication::quit, Qt::QueuedConnection);
    }};
    application.exec();
    clientThread.join();
    service.close();
    assert(passed);
}
