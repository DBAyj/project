#include "astra/render/ProjectionOutput.h"

#include "astra/common/ErrorCode.h"

#include <QtGlobal>

namespace astra::render {

bool Resolution::isValid() const { return width > 0 && height > 0 && refreshRateHz > 0.0; }

WindowProjectionOutput::WindowProjectionOutput(QString outputId)
    : outputId_(std::move(outputId))
{
}

OutputResult WindowProjectionOutput::initialize(const Resolution &resolution)
{
    state_ = OutputState::Initializing;
    if (!connected_ || !resolution.isValid()) {
        return failure(static_cast<int>(astra::common::ErrorCode::ProjectionOutputInitializationFailed),
                       QStringLiteral("Projection output is unavailable"));
    }
    resolution_ = resolution;
    frame_ = QImage(resolution.width, resolution.height, QImage::Format_RGBA8888);
    if (frame_.isNull()) {
        return failure(static_cast<int>(astra::common::ErrorCode::ProjectionOutputInitializationFailed),
                       QStringLiteral("Projection output frame allocation failed"));
    }
    frame_.fill(Qt::black);
    state_ = OutputState::Ready;
    return success();
}

OutputResult WindowProjectionOutput::present(const QImage &frame)
{
    if (!connected_) {
        return failure(static_cast<int>(astra::common::ErrorCode::ProjectionOutputDisconnected),
                       QStringLiteral("Projection output disconnected"));
    }
    if (frame_.isNull() || frame.isNull() || frame.size() != frame_.size()) {
        return failure(static_cast<int>(astra::common::ErrorCode::ProjectionOutputPresentFailed),
                       QStringLiteral("Projection frame does not match output resolution"));
    }
    frame_ = frame.convertToFormat(QImage::Format_RGBA8888);
    state_ = OutputState::Presenting;
    return success();
}

OutputResult WindowProjectionOutput::clear()
{
    if (frame_.isNull()) {
        return failure(static_cast<int>(astra::common::ErrorCode::ProjectionSafeClearFailed),
                       QStringLiteral("Projection output is not initialized"));
    }
    frame_.fill(Qt::black);
    state_ = OutputState::Cleared;
    return success();
}

OutputResult WindowProjectionOutput::shutdown()
{
    if (!frame_.isNull()) frame_.fill(Qt::black);
    state_ = OutputState::Stopped;
    return success();
}

OutputState WindowProjectionOutput::state() const { return state_; }

Resolution WindowProjectionOutput::resolution() const { return resolution_; }

double WindowProjectionOutput::refreshRateHz() const { return resolution_.refreshRateHz; }

QString WindowProjectionOutput::outputId() const { return outputId_; }

QImage WindowProjectionOutput::frame() const { return frame_; }

void WindowProjectionOutput::setConnected(bool connected)
{
    connected_ = connected;
    if (!connected_) state_ = OutputState::Error;
}

OutputResult WindowProjectionOutput::success() const { return {true, 0, {}}; }

OutputResult WindowProjectionOutput::failure(int errorCode, const QString &message)
{
    state_ = OutputState::Error;
    return {false, errorCode, message};
}

} // namespace astra::render
