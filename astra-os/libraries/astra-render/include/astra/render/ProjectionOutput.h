#pragma once

#include <QImage>
#include <QString>

namespace astra::render {

enum class OutputState { Uninitialized, Initializing, Ready, Presenting, Cleared, Stopped, Error };

struct Resolution {
    int width {0};
    int height {0};
    double refreshRateHz {0.0};

    bool isValid() const;
    bool operator==(const Resolution &) const = default;
};

struct OutputResult {
    bool ok {false};
    int errorCode {0};
    QString message;
};

class ProjectionOutput {
public:
    virtual ~ProjectionOutput() = default;
    virtual OutputResult initialize(const Resolution &resolution) = 0;
    virtual OutputResult present(const QImage &frame) = 0;
    virtual OutputResult clear() = 0;
    virtual OutputResult shutdown() = 0;
    virtual OutputState state() const = 0;
    virtual Resolution resolution() const = 0;
    virtual double refreshRateHz() const = 0;
};

class WindowProjectionOutput final : public ProjectionOutput {
public:
    explicit WindowProjectionOutput(QString outputId);

    OutputResult initialize(const Resolution &resolution) override;
    OutputResult present(const QImage &frame) override;
    OutputResult clear() override;
    OutputResult shutdown() override;
    OutputState state() const override;
    Resolution resolution() const override;
    double refreshRateHz() const override;

    QString outputId() const;
    QImage frame() const;
    void setConnected(bool connected);

private:
    OutputResult success() const;
    OutputResult failure(int errorCode, const QString &message);

    QString outputId_;
    Resolution resolution_;
    OutputState state_ {OutputState::Uninitialized};
    bool connected_ {true};
    QImage frame_;
};

} // namespace astra::render
