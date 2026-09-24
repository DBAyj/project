# P3 Observer Simulation

`SimulatedObserverTracker` owns one `primary-observer` pose in `WORLD_SIMULATED`, with position, yaw/pitch/roll, `TRACKING` state, and confidence 1.0. Move and reset commands validate finite configured bounds.

Shell maps pose changes to projection-camera offsets for dynamic perspective demonstration. This is a debug simulation, not eye tracking, head tracking, or free-viewpoint holography.
