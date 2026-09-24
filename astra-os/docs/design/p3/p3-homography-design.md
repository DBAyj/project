# P3 Homography Design

Four ordered image points map to the unit surface rectangle with OpenCV perspective transforms. The calculator rejects duplicate, self-intersecting, tiny, out-of-range, non-finite, singular, ill-conditioned, or high-reprojection-error input. It stores forward and inverse 3x3 matrices in row-major order with explicit coordinate-system metadata.

Calibration profiles include source resolution, points, matrices, reprojection error, profile version, and timestamps. Resolution or profile-version changes invalidate the mapping and require recalculation before external content can resume.
