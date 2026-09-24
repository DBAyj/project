# P2 Fallback Design

If the deterministic model fails, the rule engine produces results with `fallback_used=true`. If rules fail, the model may produce results. If both fail, the service returns 2004 with `REJECT`. Cache never masks engine health for safety-critical actions.

If the service process or socket is unavailable, Shell's P1 fallback allows only stop projection, hide projection content, and show system status. It denies projection starts, privacy expansion, model switching, and all unknown requests. The fallback result is visible and audited.
