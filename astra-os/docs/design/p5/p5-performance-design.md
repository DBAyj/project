# P5 Performance Design

Layout, input, focus, state updates, and layer generation expose monotonic
timings. The Mac target is layout P95 <= 8 ms for 100 components, input <= 4 ms,
focus <= 3 ms, state <= 5 ms, layer mapping <= 5 ms, first frame <= 1200 ms,
60 FPS target and stable 30 FPS floor. Reports use measured fixture runs.
