#include "astra/common/ErrorCodeRegistry.h"

#include <QSet>

#include <cassert>

int main()
{
    const auto descriptors = astra::common::ErrorCodeRegistry::all();
    QSet<int> codes;
    QSet<QString> symbols;
    for (const auto &descriptor : descriptors) {
        assert(!codes.contains(descriptor.code));
        assert(!symbols.contains(descriptor.symbol));
        assert(!descriptor.messageZh.isEmpty());
        assert(!descriptor.messageEn.isEmpty());
        assert(!descriptor.module.isEmpty());
        codes.insert(descriptor.code);
        symbols.insert(descriptor.symbol);
    }
    const auto privacyDenied = astra::common::ErrorCodeRegistry::lookup(4301);
    assert(privacyDenied.module == QStringLiteral("astra-policy"));
    assert(privacyDenied.auditRequired);
    for (int code = 5101; code <= 5905; ++code) {
        const bool isP5Code = (code >= 5101 && code <= 5107) || (code >= 5201 && code <= 5206)
            || (code >= 5301 && code <= 5305) || (code >= 5401 && code <= 5406)
            || (code >= 5501 && code <= 5504) || (code >= 5601 && code <= 5605)
            || (code >= 5701 && code <= 5704) || (code >= 5801 && code <= 5804)
            || (code >= 5901 && code <= 5905);
        if (isP5Code) {
            assert(astra::common::ErrorCodeRegistry::lookup(code).code == code);
        }
    }
    assert(astra::common::ErrorCodeRegistry::lookup(9998).code == 9001);
    for (int code = 2001; code <= 2010; ++code) assert(astra::common::ErrorCodeRegistry::lookup(code).code == code);
    for (int code = 2101; code <= 2104; ++code) assert(astra::common::ErrorCodeRegistry::lookup(code).code == code);
    for (int code = 2201; code <= 2204; ++code) assert(astra::common::ErrorCodeRegistry::lookup(code).code == code);
    for (int code = 2301; code <= 2304; ++code) assert(astra::common::ErrorCodeRegistry::lookup(code).code == code);
    for (int code = 3001; code <= 3009; ++code) assert(astra::common::ErrorCodeRegistry::lookup(code).code == code);
    for (int code = 3101; code <= 3105; ++code) assert(astra::common::ErrorCodeRegistry::lookup(code).code == code);
    for (int code = 3201; code <= 3208; ++code) assert(astra::common::ErrorCodeRegistry::lookup(code).code == code);
    for (int code = 3301; code <= 3306; ++code) assert(astra::common::ErrorCodeRegistry::lookup(code).code == code);
    for (int code = 3401; code <= 3404; ++code) assert(astra::common::ErrorCodeRegistry::lookup(code).code == code);
    for (int code = 3501; code <= 3504; ++code) assert(astra::common::ErrorCodeRegistry::lookup(code).code == code);
    assert(astra::common::ErrorCodeRegistry::lookup(3001).symbol == QStringLiteral("SPATIAL_SERVICE_NOT_STARTED"));
    assert(astra::common::ErrorCodeRegistry::lookup(3303).symbol == QStringLiteral("SPATIAL_ANCHOR_NOT_FOUND"));
    assert(!astra::common::ErrorCodeRegistry::exportJson().isEmpty());
}
