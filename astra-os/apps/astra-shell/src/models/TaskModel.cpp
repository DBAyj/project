#include "models/TaskModel.h"

#include <algorithm>

namespace astra::shell {

TaskModel::TaskModel(int limit, QObject *parent) : QAbstractListModel(parent) { setLimit(limit); }

int TaskModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : records_.size();
}

QVariant TaskModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= records_.size()) return {};
    const TaskRecord &record = records_.at(index.row());
    switch (role) {
    case RawTextRole: return record.rawText;
    case IntentRole: return record.intent;
    case TargetSpaceRole: return record.targetSpace;
    case PrivacyLevelRole: return record.privacyLevel;
    case ExecutionStatusRole: return record.executionStatus;
    case ErrorMessageRole: return record.errorMessage;
    case RequestIdRole: return record.requestId;
    case TimestampRole: return record.timestamp;
    default: return {};
    }
}

QHash<int, QByteArray> TaskModel::roleNames() const
{
    return {{RawTextRole, "rawText"}, {IntentRole, "intent"}, {TargetSpaceRole, "targetSpace"},
            {PrivacyLevelRole, "privacyLevel"}, {ExecutionStatusRole, "executionStatus"},
            {ErrorMessageRole, "errorMessage"}, {RequestIdRole, "requestId"}, {TimestampRole, "timestamp"}};
}

int TaskModel::count() const { return records_.size(); }
int TaskModel::limit() const { return limit_; }

void TaskModel::setLimit(int limit)
{
    limit_ = std::max(1, limit);
    while (records_.size() > limit_) {
        beginRemoveRows({}, 0, 0);
        records_.removeFirst();
        endRemoveRows();
    }
}

void TaskModel::append(const TaskRecord &record)
{
    if (records_.size() == limit_) {
        beginRemoveRows({}, 0, 0);
        records_.removeFirst();
        endRemoveRows();
    }
    const int row = records_.size();
    beginInsertRows({}, row, row);
    records_.append(record);
    endInsertRows();
    emit countChanged();
}

} // namespace astra::shell
