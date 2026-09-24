#include "clients/ProjectionServiceClient.h"

#include <QJsonObject>

#include <cassert>

int main()
{
    const QJsonObject success {{QStringLiteral("jsonrpc"), QStringLiteral("2.0")},
                               {QStringLiteral("id"), QStringLiteral("test")},
                               {QStringLiteral("result"), QJsonObject {{QStringLiteral("session_id"), QStringLiteral("session-001")},
                                                                        {QStringLiteral("state"), QStringLiteral("RENDERING")},
                                                                        {QStringLiteral("frame_id"), 1}}}};
    const auto decoded = astra::shell::ProjectionServiceClient::decodeResponse(success);
    assert(decoded.ok);
    assert(decoded.sessionId == QStringLiteral("session-001"));
    assert(decoded.state == QStringLiteral("RENDERING"));

    const QJsonObject denied {{QStringLiteral("jsonrpc"), QStringLiteral("2.0")},
                              {QStringLiteral("id"), QStringLiteral("test")},
                              {QStringLiteral("error"), QJsonObject {{QStringLiteral("code"), 5002}, {QStringLiteral("message"), QStringLiteral("Capability token is invalid")}}}};
    const auto rejected = astra::shell::ProjectionServiceClient::decodeResponse(denied);
    assert(!rejected.ok);
    assert(rejected.errorCode == 5002);
}
