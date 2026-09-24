# P3 Calibration Design

Calibration states are idle, collecting, preview, confirmed, cancelled, and failed. Points are collected strictly as top-left, top-right, bottom-right, bottom-left. Each addition validates image bounds and uniqueness; confirmation validates convexity, intersection, area, invertibility, and reprojection error.

Confirmed profiles are saved only under `runtime/spatial/calibration` when configured. Cancellation and failure preserve the previously active target. UI shows a corrected preview summary before confirmation.
