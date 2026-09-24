#pragma once

#include <QString>
#include <QStringList>
#include <QVector>

#include <functional>

namespace astra::render {

struct RenderPass {
    QString id;
    QStringList dependsOn;
};

struct RenderGraphResult {
    bool ok {false};
    int errorCode {0};
    QStringList order;
    QString message;
};

class RenderGraph final {
public:
    explicit RenderGraph(QVector<RenderPass> passes);

    RenderGraphResult topologicalOrder() const;
    RenderGraphResult run(bool outputAvailable, const std::function<bool(const RenderPass &)> &execute) const;
    static RenderGraph fixedP4Pipeline();

private:
    QVector<RenderPass> passes_;
};

} // namespace astra::render
