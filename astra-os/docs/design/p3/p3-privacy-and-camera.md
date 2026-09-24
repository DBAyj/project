# P3 Camera Privacy

The Phone Display explains camera purpose before the first start and shows an active indicator while capture runs. Permission denial is remembered for the current process to avoid repeated prompts and activates simulation fallback. Stop and process shutdown release capture.

Raw frames remain memory-only by default. Debug saving requires explicit configuration, writes only under `runtime/spatial/debug`, emits a separate audit event, and never targets Photos or arbitrary user directories. Audit stores metadata and hashes, never pixels.
