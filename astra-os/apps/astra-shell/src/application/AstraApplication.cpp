#include "application/AstraApplication.h"

#include "controllers/ShellController.h"
#include "services/ConfigurationService.h"

#include <QDir>
#include <QDebug>
#include <QElapsedTimer>
#include <QGuiApplication>
#include <QQuickWindow>
#include <QSGRendererInterface>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QSet>
#include <QTimer>
#include <QVariant>
#include <QWindow>

namespace astra::shell {
namespace {

QString graphicsApiName(QSGRendererInterface::GraphicsApi api)
{
    switch (api) {
    case QSGRendererInterface::Metal: return QStringLiteral("Metal");
    case QSGRendererInterface::OpenGL: return QStringLiteral("OpenGL");
    case QSGRendererInterface::Vulkan: return QStringLiteral("Vulkan");
    case QSGRendererInterface::Software: return QStringLiteral("Software");
    case QSGRendererInterface::Null: return QStringLiteral("Null");
    default: return QStringLiteral("Unknown");
    }
}

void logGraphicsBackend(const QQuickWindow *window)
{
    const auto api = window->rendererInterface()->graphicsApi();
    qInfo().noquote() << "graphics_api=" + graphicsApiName(api)
                      << "renderer_interface=RHI"
                      << "rhi_backend=" + graphicsApiName(api)
                      << "window_title=" + window->title();
}

} // namespace

AstraApplication::AstraApplication(QGuiApplication &application) : application_(application) {}
AstraApplication::~AstraApplication() = default;

QString AstraApplication::resolveProjectRoot(const QString &configuredRoot, const QString &fallbackDirectory)
{
    return QDir(configuredRoot.isEmpty() ? fallbackDirectory : configuredRoot).absolutePath();
}

QString AstraApplication::configurationPath(const QString &projectRoot)
{
    return QDir(projectRoot).filePath(QStringLiteral("config/astra.example.yaml"));
}

QString AstraApplication::runtimePath(const QString &projectRoot, const QString &relativePath)
{
    return QDir(projectRoot).filePath(relativePath);
}

bool AstraApplication::hasExpectedRootWindows(const QList<QObject *> &rootObjects)
{
    if (rootObjects.size() != 2) return false;
    QSet<QString> titles;
    for (QObject *root : rootObjects) {
        const auto *window = qobject_cast<QQuickWindow *>(root);
        if (window == nullptr) return false;
        titles.insert(window->title());
    }
    return titles == QSet<QString> {QStringLiteral("AstraOS Phone Display"), QStringLiteral("AstraOS Projection Display")};
}

bool AstraApplication::initialize()
{
    auto firstFrameTimer = std::make_shared<QElapsedTimer>();
    firstFrameTimer->start();
    const QString projectRoot = resolveProjectRoot(qEnvironmentVariable("ASTRA_PROJECT_ROOT"), QDir::currentPath());
    const ConfigurationLoadResult configuration = ConfigurationService::load(
        configurationPath(projectRoot), QDir(projectRoot).filePath(QStringLiteral("schemas/application-config.schema.json")));
    controller_ = std::make_unique<ShellController>(configuration.config,
                                                    runtimePath(projectRoot, configuration.config.auditPath));
    const bool loadFailed = configuration.warning.contains(QStringLiteral("unavailable"));
    controller_->setConfigurationStatus(configuration.valid ? QStringLiteral("VALID")
                                                       : (loadFailed ? QStringLiteral("LOAD_FAILED_FALLBACK_ACTIVE")
                                                                     : QStringLiteral("INVALID_FALLBACK_ACTIVE")));
    controller_->setConfigurationDiagnostics(configuration.warning.isEmpty() ? QStringList {} : QStringList {configuration.warning},
                                             configuration.usingFallbackConfiguration);
    controller_->recordConfigurationResult(configuration.valid);

    engine_ = std::make_unique<QQmlApplicationEngine>();
    engine_->rootContext()->setContextProperty(QStringLiteral("shellController"), controller_.get());
    const QVariantMap rootProperties {{QStringLiteral("controller"), QVariant::fromValue(static_cast<QObject *>(controller_.get()))}};
    engine_->setInitialProperties(rootProperties);
    engine_->loadFromModule("Astra.Shell", "PhoneWindow");
    engine_->setInitialProperties(rootProperties);
    engine_->loadFromModule("Astra.Shell", "ProjectionWindow");
    if (!hasExpectedRootWindows(engine_->rootObjects())) {
        controller_->recordApplicationInitializationFailure();
        return false;
    }

    QTimer::singleShot(0, &application_, [this, firstFrameTimer] {
        for (QWindow *topLevelWindow : application_.topLevelWindows()) {
            auto *window = qobject_cast<QQuickWindow *>(topLevelWindow);
            if (window == nullptr) continue;
            QObject::connect(window, &QQuickWindow::sceneGraphInitialized, window, [window] {
                logGraphicsBackend(window);
            }, Qt::DirectConnection);
            QObject::connect(window, &QQuickWindow::frameSwapped, window, [window, firstFrameTimer] {
                if (window->property("p5_first_frame_logged").toBool()) return;
                const QString workspaceName = window->title() == QStringLiteral("AstraOS Phone Display")
                    ? QStringLiteral("phoneSpatialWorkspace") : QStringLiteral("p4FixtureOutput");
                const QObject *workspace = window->findChild<QObject *>(workspaceName);
                window->setProperty("p5_first_frame_logged", true);
                qInfo().noquote() << "p5_ui_first_frame=true"
                                  << QStringLiteral("elapsed_ms=%1").arg(firstFrameTimer->elapsed())
                                  << "window_visible=" + QString(window->isVisible() ? QStringLiteral("true") : QStringLiteral("false"))
                                  << "workspace_visible=" + QString(workspace != nullptr && workspace->property("visible").toBool()
                                                                           ? QStringLiteral("true") : QStringLiteral("false"))
                                  << "window_title=" + window->title();
            }, Qt::DirectConnection);
            QObject::connect(window, &QQuickWindow::afterRendering, window, [this, window] {
                if (window->title() != QStringLiteral("AstraOS Projection Display") || !controller_->p4ProjectionActive()
                    || window->property("p4_output_first_frame_logged").toBool()) return;
                const QObject *frame = window->findChild<QObject *>(QStringLiteral("p4FixtureOutput"));
                if (frame == nullptr || frame->property("status").toInt() != 1) return;
                window->setProperty("p4_output_first_frame_logged", true);
                qInfo().noquote() << "p4_projection_output_first_frame=true"
                                  << "window_visible=" + QString(window->isVisible() ? QStringLiteral("true") : QStringLiteral("false"))
                                  << "window_title=" + window->title()
                                  << "p3_integration_status=P3_REAL_SPATIAL_INTEGRATION_NOT_VERIFIED";
            }, Qt::DirectConnection);
            QTimer::singleShot(250, window, [window] {
                if (window->isSceneGraphInitialized()) logGraphicsBackend(window);
            });
        }
        if (qEnvironmentVariableIntValue("ASTRA_P4_DEMO_ON_START") == 1) {
            QTimer::singleShot(100, controller_.get(), [this] { controller_->startP4FixtureDemo(); });
        }
        if (qEnvironmentVariableIntValue("ASTRA_P5_GRAPHICS_PROBE") == 1) {
            QTimer::singleShot(5000, controller_.get(), [this] {
                controller_->enterFullscreen();
                qInfo().noquote() << "p5_fullscreen_entered=true";
            });
            QTimer::singleShot(7000, controller_.get(), [this] {
                controller_->exitFullscreen();
                qInfo().noquote() << "p5_fullscreen_exited=true";
            });
        }
        if (application_.topLevelWindows().isEmpty()) {
            qWarning().noquote() << "graphics_diagnostics=no_top_level_windows";
        }
    });

    controller_->recordApplicationStarted();
    if (qEnvironmentVariableIntValue("ASTRA_P3_MODE") == 1) controller_->startSpatial();
    if (configuration.config.startFullscreen) controller_->enterFullscreen();
    QObject::connect(&application_, &QGuiApplication::aboutToQuit, [this] { controller_->recordApplicationStopped(); });
    return true;
}

} // namespace astra::shell
