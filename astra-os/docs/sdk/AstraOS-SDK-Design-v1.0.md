# AstraOS SDK Design v1.0

## SDK Layers and APIs

The future SDK has contract definitions, generated client bindings, application lifecycle APIs, AI Skill APIs, spatial APIs, projection APIs, capability APIs, and event APIs. Apps never link privileged implementation internals; all service calls use versioned schemas and a runtime-provided capability context.

## Security and Lifecycle

An application declares identity, requested capabilities, and version in a signed manifest. The runtime grants only approved scoped capabilities, delivers lifecycle events, mediates data access, and revokes calls on suspension or removal. Projection and cloud AI APIs require policy decisions in addition to capability grants.

## Versioning, Examples, Testing, and Signing

SDK major/minor compatibility follows [Versioning Standard](../architecture/16-versioning-standard.md). Examples will include a private task card, a policy-approved projection request, and a bounded spatial object. Developer tests validate schemas, denied calls, lifecycle transitions, and signature verification. Packaging and signing are designed here and implemented in P6.
