#include "astra/ui/UIStateStore.h"

#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTemporaryDir>
#include <QtGlobal>

using namespace astra::ui;

namespace {
void require(bool condition)
{
    if (!condition) qFatal("P5 UI state requirement failed");
}
}

int main()
{
    QTemporaryDir temporary;
    require(temporary.isValid());
    const QString path = temporary.filePath(QStringLiteral("ui-state.json"));
    UIStateStore store {path};
    UIStateSnapshot snapshot;
    snapshot.layoutMode = LayoutMode::Grid;
    const QString componentId = QStringLiteral("7abf7b1e-dc4f-4b7c-96cc-c3dd628e1ca9");
    const QString windowId = QStringLiteral("c38c0e21-96f1-4975-8cf3-fbfaa58c7cda");
    snapshot.components = {{componentId, ComponentType::TaskCard, {20.0, 30.0, 400.0, 240.0}, 2, true, true, true,
                            DisplayTarget::Phone}};
    snapshot.windows = {{windowId, componentId, {20.0, 30.0, 400.0, 240.0}, true, DisplayTarget::Phone, {}, {},
                         QStringLiteral("workspace")}};
    snapshot.visibleComponentIds = {componentId};
    snapshot.selectedTab = QStringLiteral("tasks");
    snapshot.focusRestoreComponentId = componentId;
    snapshot.panelOrder = {QStringLiteral("tasks"), QStringLiteral("system")};
    require(store.save(snapshot).ok);

    QFile persisted {path};
    require(persisted.open(QIODevice::ReadOnly));
    const QJsonObject persistedState = QJsonDocument::fromJson(persisted.readAll()).object();
    persisted.close();
    require(!persistedState.value(QStringLiteral("components")).toArray().at(0).toObject().contains(QStringLiteral("privacy_level")));
    require(!persistedState.value(QStringLiteral("windows")).toArray().at(0).toObject().contains(QStringLiteral("privacy_level")));

    const auto loaded = store.load();
    require(loaded.ok);
    require(loaded.snapshot.layoutMode == LayoutMode::Grid);
    require(loaded.snapshot.windows.at(0).bounds.x == 20.0);
    require(loaded.snapshot.selectedTab == QStringLiteral("tasks"));

    QJsonObject injectedState = persistedState;
    QJsonArray injectedComponents = injectedState.value(QStringLiteral("components")).toArray();
    QJsonObject injectedComponent = injectedComponents.at(0).toObject();
    injectedComponent.insert(QStringLiteral("privacy_level"), QStringLiteral("PUBLIC"));
    injectedComponents[0] = injectedComponent;
    injectedState.insert(QStringLiteral("components"), injectedComponents);
    QFile injected {path};
    require(injected.open(QIODevice::WriteOnly | QIODevice::Truncate));
    require(injected.write(QJsonDocument {injectedState}.toJson()) > 0);
    injected.close();
    const auto privacyRejected = store.load();
    require(!privacyRejected.ok && privacyRejected.errorCode == 5803);

    QJsonObject legacyState = persistedState;
    legacyState.insert(QStringLiteral("schema_version"), QStringLiteral("1.0"));
    QFile legacy {path};
    require(legacy.open(QIODevice::WriteOnly | QIODevice::Truncate));
    require(legacy.write(QJsonDocument {legacyState}.toJson()) > 0);
    legacy.close();
    const auto legacyRejected = store.load();
    require(!legacyRejected.ok && legacyRejected.errorCode == 5804);

    QFile tampered {path};
    require(tampered.open(QIODevice::WriteOnly | QIODevice::Truncate));
    require(tampered.write("{\"schema_version\":\"1.0\",\"secret\":\"leak\"}") > 0);
    tampered.close();
    const auto rejected = store.load();
    require(!rejected.ok && rejected.errorCode == 5803);
}
