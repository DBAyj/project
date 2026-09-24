# ADR-0002: Use JSON-RPC over Unix Socket

- Status: Accepted
- Date: 2026-07-14

## Context

The first generation has a small set of local processes and needs inspectable request/response communication without an infrastructure broker.

## Decision

Use JSON-RPC 2.0 over Unix Domain Sockets for local service calls and WebSocket for real-time events.

## Reasons and Alternatives

This is portable, debuggable, schema-friendly, and appropriate for the local process count. Kafka, RabbitMQ, service registries, and Kubernetes-oriented networking are rejected as disproportionate.

## Impact, Risks, and Rollback

High-frequency frame traffic is excluded and will use a future shared-memory contract. A versioned protocol envelope permits a future transport migration without rewriting service semantics.
