# P3 Coordinate Systems

Supported names are `IMAGE_PIXEL`, `IMAGE_NORMALIZED`, `PHONE_VIEW`, `PROJECTION_VIEW`, `SURFACE_LOCAL`, and `WORLD_SIMULATED`. Pixel and view origins are top-left with positive X right and positive Y down. Surface-local coordinates are normalized from the selected top-left corner. Simulated world uses a right-handed meter space: X right, Y up, Z toward the observer.

Every point collection and transform carries a source and target coordinate system. Unsupported or implicit conversions return 3403/3404. Resolution-dependent mappings are invalidated and recomputed when source or target dimensions change.
