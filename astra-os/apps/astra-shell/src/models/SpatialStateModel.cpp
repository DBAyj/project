#include "models/SpatialStateModel.h"

namespace astra::shell {

SpatialStateModel::SpatialStateModel(QObject *parent) : QObject(parent) {}

QString SpatialStateModel::state() const { return state_; }
QString SpatialStateModel::inputSource() const { return inputSource_; }
QString SpatialStateModel::targetState() const { return targetState_; }
QString SpatialStateModel::quality() const { return quality_; }
QString SpatialStateModel::warning() const { return warning_; }
int SpatialStateModel::lastErrorCode() const { return lastErrorCode_; }
QJsonArray SpatialStateModel::calibrationCorners() const { return calibrationCorners_; }

void SpatialStateModel::apply(const QJsonObject &value)
{
    state_ = value.value(QStringLiteral("state")).toString(state_);
    inputSource_ = value.value(QStringLiteral("input_source")).toString(inputSource_);
    quality_ = value.value(QStringLiteral("quality")).toString(quality_);
    warning_ = value.value(QStringLiteral("warning")).toString();
    lastErrorCode_ = value.value(QStringLiteral("last_error_code")).isNull() ? 0 : value.value(QStringLiteral("last_error_code")).toInt();
    const QJsonObject target = value.value(QStringLiteral("target")).toObject();
    targetState_ = target.value(QStringLiteral("state")).toString(QStringLiteral("NONE"));
    calibrationCorners_ = target.value(QStringLiteral("corners")).toArray();
    emit changed();
}

} // namespace astra::shell
