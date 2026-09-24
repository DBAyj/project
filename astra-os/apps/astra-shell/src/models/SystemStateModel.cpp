#include "models/SystemStateModel.h"

#include <QSysInfo>
#include <QtGlobal>

namespace astra::shell {

SystemStateModel::SystemStateModel(QObject *parent) : QObject(parent) {}

QString SystemStateModel::systemVersion() const { return QStringLiteral("0.3.0-alpha.1"); }
QString SystemStateModel::runtimePlatform() const { return QStringLiteral("macOS development host"); }
QString SystemStateModel::cpuArchitecture() const { return QSysInfo::currentCpuArchitecture(); }
QString SystemStateModel::qtVersion() const { return QString::fromLatin1(qVersion()); }
QString SystemStateModel::projectionStatus() const { return projectionStatus_; }
QString SystemStateModel::spatialStatus() const { return spatialStatus_; }
QString SystemStateModel::auditStatus() const { return auditStatus_; }
QString SystemStateModel::configurationStatus() const { return configurationStatus_; }
int SystemStateModel::configurationWarningCount() const { return configurationWarnings_.size(); }
QStringList SystemStateModel::configurationWarnings() const { return configurationWarnings_; }
bool SystemStateModel::usingFallbackConfiguration() const { return usingFallbackConfiguration_; }

void SystemStateModel::setProjectionStatus(const QString &status)
{
    if (projectionStatus_ == status) return;
    projectionStatus_ = status;
    emit statusChanged();
}

void SystemStateModel::setSpatialStatus(const QString &status)
{
    if (spatialStatus_ == status) return;
    spatialStatus_ = status;
    emit statusChanged();
}

void SystemStateModel::setAuditStatus(const QString &status)
{
    if (auditStatus_ == status) return;
    auditStatus_ = status;
    emit statusChanged();
}

void SystemStateModel::setConfigurationStatus(const QString &status)
{
    if (configurationStatus_ == status) return;
    configurationStatus_ = status;
    emit statusChanged();
}

void SystemStateModel::setConfigurationDiagnostics(const QStringList &warnings, bool usingFallback)
{
    if (configurationWarnings_ == warnings && usingFallbackConfiguration_ == usingFallback) return;
    configurationWarnings_ = warnings;
    usingFallbackConfiguration_ = usingFallback;
    emit statusChanged();
}

} // namespace astra::shell
