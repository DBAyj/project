#pragma once

#include <QObject>
#include <QString>
#include <QStringList>

namespace astra::shell {

class SystemStateModel final : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString systemVersion READ systemVersion CONSTANT)
    Q_PROPERTY(QString runtimePlatform READ runtimePlatform CONSTANT)
    Q_PROPERTY(QString cpuArchitecture READ cpuArchitecture CONSTANT)
    Q_PROPERTY(QString qtVersion READ qtVersion CONSTANT)
    Q_PROPERTY(QString projectionStatus READ projectionStatus NOTIFY statusChanged)
    Q_PROPERTY(QString spatialStatus READ spatialStatus NOTIFY statusChanged)
    Q_PROPERTY(QString auditStatus READ auditStatus NOTIFY statusChanged)
    Q_PROPERTY(QString configurationStatus READ configurationStatus NOTIFY statusChanged)
    Q_PROPERTY(int configurationWarningCount READ configurationWarningCount NOTIFY statusChanged)
    Q_PROPERTY(QStringList configurationWarnings READ configurationWarnings NOTIFY statusChanged)
    Q_PROPERTY(bool usingFallbackConfiguration READ usingFallbackConfiguration NOTIFY statusChanged)

public:
    explicit SystemStateModel(QObject *parent = nullptr);

    QString systemVersion() const;
    QString runtimePlatform() const;
    QString cpuArchitecture() const;
    QString qtVersion() const;
    QString projectionStatus() const;
    QString spatialStatus() const;
    QString auditStatus() const;
    QString configurationStatus() const;
    int configurationWarningCount() const;
    QStringList configurationWarnings() const;
    bool usingFallbackConfiguration() const;

    void setProjectionStatus(const QString &status);
    void setSpatialStatus(const QString &status);
    void setAuditStatus(const QString &status);
    void setConfigurationStatus(const QString &status);
    void setConfigurationDiagnostics(const QStringList &warnings, bool usingFallback);

signals:
    void statusChanged();

private:
    QString projectionStatus_ {QStringLiteral("IDLE")};
    QString spatialStatus_ {QStringLiteral("STOPPED")};
    QString auditStatus_ {QStringLiteral("READY")};
    QString configurationStatus_ {QStringLiteral("PENDING")};
    QStringList configurationWarnings_;
    bool usingFallbackConfiguration_ {false};
};

} // namespace astra::shell
