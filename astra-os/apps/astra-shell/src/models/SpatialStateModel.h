#pragma once

#include <QJsonObject>
#include <QJsonArray>
#include <QObject>

namespace astra::shell {

class SpatialStateModel final : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString state READ state NOTIFY changed)
    Q_PROPERTY(QString inputSource READ inputSource NOTIFY changed)
    Q_PROPERTY(QString targetState READ targetState NOTIFY changed)
    Q_PROPERTY(QString quality READ quality NOTIFY changed)
    Q_PROPERTY(QString warning READ warning NOTIFY changed)
    Q_PROPERTY(int lastErrorCode READ lastErrorCode NOTIFY changed)

public:
    explicit SpatialStateModel(QObject *parent = nullptr);

    QString state() const;
    QString inputSource() const;
    QString targetState() const;
    QString quality() const;
    QString warning() const;
    int lastErrorCode() const;
    QJsonArray calibrationCorners() const;
    void apply(const QJsonObject &value);

signals:
    void changed();

private:
    QString state_ {QStringLiteral("STOPPED")};
    QString inputSource_ {QStringLiteral("SIMULATION")};
    QString targetState_ {QStringLiteral("NONE")};
    QString quality_ {QStringLiteral("UNUSABLE")};
    QString warning_;
    int lastErrorCode_ {0};
    QJsonArray calibrationCorners_;
};

} // namespace astra::shell
