# P3 Frame Source Design

`FrameSource` exposes `start`, `stop`, `isRunning`, and `latestFrame`. Implementations are camera, PNG/JPEG image, video file, and simulation. Each frame has a UUID, UTC timestamp, source kind, dimensions, pixel format, `IMAGE_PIXEL` coordinate system, and an in-memory `cv::Mat` that is never serialized into audit.

Camera uses Qt Multimedia device enumeration and the portable OpenCV capture adapter, which materializes only the current frame in memory. Image and video validate project-supplied paths and formats. Simulation generates desk, wall, oblique, occluded, empty, and multi-surface frames. Stop is idempotent and releases device/file resources.
