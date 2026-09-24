#pragma once

#include <QJsonObject>
#include <QObject>
#include <QVariantList>

namespace astra::shell {

class SpatialUIStateModel final : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString status READ status NOTIFY changed)
    Q_PROPERTY(int componentCount READ componentCount NOTIFY changed)
    Q_PROPERTY(int windowCount READ windowCount NOTIFY changed)
    Q_PROPERTY(QString focusComponentId READ focusComponentId NOTIFY changed)
    Q_PROPERTY(bool projectionSafe READ projectionSafe NOTIFY changed)
    Q_PROPERTY(bool reduceMotion READ reduceMotion NOTIFY changed)
    Q_PROPERTY(bool highContrast READ highContrast NOTIFY changed)
    Q_PROPERTY(QString p4ReleaseStatus READ p4ReleaseStatus NOTIFY changed)
    Q_PROPERTY(QVariantList components READ components NOTIFY changed)

public:
    explicit SpatialUIStateModel(QObject *parent = nullptr);
    [[nodiscard]] QString status() const;
    [[nodiscard]] int componentCount() const;
    [[nodiscard]] int windowCount() const;
    [[nodiscard]] QString focusComponentId() const;
    [[nodiscard]] bool projectionSafe() const;
    [[nodiscard]] bool reduceMotion() const;
    [[nodiscard]] bool highContrast() const;
    [[nodiscard]] QString p4ReleaseStatus() const;
    [[nodiscard]] QVariantList components() const;
    Q_INVOKABLE [[nodiscard]] QVariantList componentsForDisplay(const QString &displayTarget) const;
    void applyStatus(const QJsonObject &status);
    void applyComponents(const QJsonObject &result);
    void setOffline();

signals:
    void changed();

private:
    QString status_ {QStringLiteral("OFFLINE")};
    int componentCount_ {0};
    int windowCount_ {0};
    QString focusComponentId_;
    bool projectionSafe_ {true};
    bool reduceMotion_ {false};
    bool highContrast_ {false};
    QString p4ReleaseStatus_ {QStringLiteral("P4_RELEASE_BASELINE_FINAL")};
    QVariantList components_;
};

} // namespace astra::shell
