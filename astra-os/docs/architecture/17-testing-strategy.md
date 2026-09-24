# 17 Testing Strategy

The required layers are unit, contract, component, integration, end-to-end, performance, security, regression, and hardware-adaptation tests. Protocols require schema tests; the error registry requires range and uniqueness tests; permissions require both allow and deny tests; configuration requires valid and invalid tests; and each process requires a health-check test.

Defect fixes start with a reproducing test. Empty assertions and skip-based success are prohibited. Each phase must publish concrete acceptance evidence. P0/P0.5 uses [documentation verification](../../scripts/verify_document_baseline.py); later phases add executable service and hardware acceptance tests.
