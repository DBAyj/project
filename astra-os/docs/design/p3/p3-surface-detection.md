# P3 Surface Detection

The deterministic pipeline resizes, normalizes brightness, converts to grayscale, blurs, applies Canny edges and morphology, then extracts contours. Quadrilateral candidates must be convex, within configured area/aspect bounds, outside degenerate borders, and have clockwise ordered corners.

Scoring combines geometry, area, stability, position, type, border, and occlusion signals. `EXCELLENT` and `GOOD` may auto-select, `FAIR` requires confirmation, and lower qualities reject. Classification is a bounded geometry/color heuristic for fixtures and must not be described as general semantic understanding.
