# P5 UI State Persistence

The store persists only window geometry, visibility, selection, layout mode,
focus restoration target, and panel order to a versioned JSON snapshot. Privacy
classifications are never persisted; restore reclassifies components through
`astra-policy` from the configured default privacy level, and windows inherit
that decision. Restored state cannot regain public projection without a new
active policy-classified request. The store rejects unknown
fields, incompatible versions, invalid bounds, and sensitive keys. Writes use a
temporary file plus atomic replacement. Runtime credentials, tokens, task text,
and unredacted AI input are never stored.
