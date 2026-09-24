# P2 Performance Design

Pipeline stages use monotonic timing and bounded synchronous work inside asyncio request tasks. Rules and examples are immutable snapshots; the LRU/TTL cache is bounded to 1,000 entries. No unbounded queues or background model downloads exist. JSONL audit writes are serialized.

The performance suite records average, P50, P95, P99, errors, throughput, startup time, and process memory. It measures 1,000 sequential requests, 20 concurrent requests, cached requests, rule-only latency, and complete pipeline latency. The stability path performs 10,000 requests and checks socket cleanup and audit parseability.
