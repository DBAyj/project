#pragma once

#include <QElapsedTimer>
#include <QLocalSocket>

#include <optional>

namespace astra::shell {

// Reads one newline-terminated JSON-RPC response. A single waitForReadyRead()
// may deliver only part of a large response (for example a PNG frame).
inline std::optional<QByteArray> readResponseLine(QLocalSocket &socket, int timeoutMs)
{
    QElapsedTimer timer;
    timer.start();
    while (!socket.canReadLine()) {
        const qint64 remaining = timeoutMs - timer.elapsed();
        if (remaining <= 0 || !socket.waitForReadyRead(static_cast<int>(remaining))) return std::nullopt;
    }
    return socket.readLine().trimmed();
}

} // namespace astra::shell
