#pragma once

#include "astra/ui/Types.h"

#include <QHash>

namespace astra::ui {

enum class FocusType { None, Keyboard, Pointer, Touch, GestureSimulated, System };
enum class FocusPriority { Background = 0, NormalComponent = 1, ActiveWindow = 2, Modal = 3, PrivacyConfirmation = 4, System = 5 };

struct FocusTarget {
    QString componentId;
    QString scopeId;
    bool visible {true};
    bool interactive {true};
    bool focusable {true};
    int stableOrder {0};
};

struct FocusRecord {
    QString componentId;
    FocusType type {FocusType::None};
    FocusPriority priority {FocusPriority::Background};
    QString reason;
};

class SpatialFocusManager final {
public:
    OperationResult registerTarget(const FocusTarget &target);
    OperationResult unregisterTarget(const QString &componentId);
    OperationResult setAvailable(const QString &componentId, bool available);
    OperationResult requestFocus(const QString &componentId, FocusType type, FocusPriority priority, const QString &reason);
    OperationResult releaseFocus(const QString &scopeId, const QString &reason);
    OperationResult focusNext(const QString &scopeId);
    void clear();
    [[nodiscard]] QString owner(const QString &scopeId) const;
    [[nodiscard]] FocusRecord record(const QString &scopeId) const;

private:
    QHash<QString, FocusTarget> targets_;
    QHash<QString, FocusRecord> scopes_;
};

} // namespace astra::ui
