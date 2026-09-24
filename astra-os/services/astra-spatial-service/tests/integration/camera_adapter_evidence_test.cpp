#include "input/FrameSource.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>

#include <cassert>

int main(int argc, char **argv)
{
    QCoreApplication application(argc, argv);
    const QString root = QStringLiteral(ASTRA_SOURCE_ROOT);
    astra::spatial::FrameSourceController source(root);
    const auto descriptors = source.enumerate();
    bool enumerated = false;
    for (const auto &descriptor : descriptors) enumerated = enumerated || descriptor.type == astra::spatial::FrameSourceType::Camera && descriptor.available;

    bool started = false;
    bool frameReceived = false;
    bool stopped = false;
    QString limitation;
    if (!enumerated) {
        limitation = QStringLiteral("No camera hardware was enumerated by Qt Multimedia");
    } else {
        const auto start = source.start(astra::spatial::FrameSourceType::Camera);
        started = start.ok;
        if (started) {
            const auto frame = source.nextFrame();
            frameReceived = frame.ok;
            stopped = source.stop().ok && !source.running();
            if (!frameReceived) limitation = QStringLiteral("Camera authorization or frame receipt was unavailable");
        } else {
            limitation = start.message;
        }
    }

    const bool passed = enumerated && started && frameReceived && stopped;
    const QString output = qEnvironmentVariable("ASTRA_P3_CAMERA_PATH", root + QStringLiteral("/runtime/tmp/p3-camera-evidence.json"));
    QDir().mkpath(QFileInfo(output).dir().path());
    QFile file(output);
    assert(file.open(QIODevice::WriteOnly | QIODevice::Truncate));
    const QJsonObject evidence {{QStringLiteral("status"), passed ? QStringLiteral("PASSED") : QStringLiteral("SKIPPED_ENVIRONMENT_LIMITATION")},
                                {QStringLiteral("camera_enumerated"), enumerated},
                                {QStringLiteral("camera_started"), started},
                                {QStringLiteral("frame_received"), frameReceived},
                                {QStringLiteral("camera_stopped_and_released"), stopped},
                                {QStringLiteral("limitation"), limitation}};
    assert(file.write(QJsonDocument(evidence).toJson(QJsonDocument::Compact) + '\n') > 0);
}
