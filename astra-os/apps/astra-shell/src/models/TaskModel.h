#pragma once

#include <QAbstractListModel>
#include <QString>
#include <QVector>

namespace astra::shell {

struct TaskRecord {
    QString rawText;
    QString intent;
    QString targetSpace;
    QString privacyLevel;
    QString executionStatus;
    QString errorMessage;
    QString requestId;
    QString timestamp;
};

class TaskModel final : public QAbstractListModel {
    Q_OBJECT
    Q_PROPERTY(int count READ count NOTIFY countChanged)

public:
    enum Role {
        RawTextRole = Qt::UserRole + 1,
        IntentRole,
        TargetSpaceRole,
        PrivacyLevelRole,
        ExecutionStatusRole,
        ErrorMessageRole,
        RequestIdRole,
        TimestampRole,
    };
    Q_ENUM(Role)

    explicit TaskModel(int limit = 10, QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;
    int count() const;
    int limit() const;

    void setLimit(int limit);
    void append(const TaskRecord &record);

signals:
    void countChanged();

private:
    int limit_ {10};
    QVector<TaskRecord> records_;
};

} // namespace astra::shell
