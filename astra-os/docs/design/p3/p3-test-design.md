# P3 Test Design

Confirmed public seams are frame-source lifecycle, deterministic image output, preprocessor output, candidate detector/selector, calibration/homography, coordinate mapper, scene graph, observer tracker, service state/API, strict schemas, Shell model/controller, and QML controls. Tests use known fixture geometry and expected numeric tolerances rather than reimplementing production algorithms.

CI defaults to simulation/image. Contract tests reject missing coordinate systems, malformed matrices, invalid UUID/time/enum/range, and unknown fields. Integration covers P2 intent to P3 execution and P1 lost-target safety. Camera tests are isolated and may skip only for measured environment authorization/hardware limits.
