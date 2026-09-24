# P2 Error Handling

P2 uses only the central registry: 2001-2010 for intent/service/configuration, 2101-2104 for confirmation/clarification, 2201-2204 for input security, and 2301-2304 for engine/extraction failures. JSON-RPC and HTTP map the same stable code, message, retryability, and trace ID.

Invalid input and security rejection never invoke engines. Engine failure follows the fallback matrix. Transport failure closes only the affected request. Service startup fails if configuration is invalid or another live process owns the socket. All errors return safe user messages, write redacted structured details, and keep projection state unchanged unless an explicit safe-stop action succeeds.
