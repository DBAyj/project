# P3 Scene Graph

The scene graph owns one `SpatialScene` containing projection targets, session anchors, scene objects, one simulated observer, and optional debug objects. IDs are UUIDs except the stable simulated observer ID. Mutation is serialized by the service and exposed through typed add, lookup, update, hide, suspend, remove, and reset operations.

Scene states are `EMPTY`, `INITIALIZING`, `TRACKING`, `DEGRADED`, `LOST`, `STOPPED`, and `ERROR`. Object states are `CREATED`, `ACTIVE`, `HIDDEN`, `SUSPENDED`, `REMOVED`, and `ERROR`. Unknown IDs never create implicit objects.
