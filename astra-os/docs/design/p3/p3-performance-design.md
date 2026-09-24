# P3 Performance Design

Processing keeps only the newest frame, reuses OpenCV buffers where practical, and avoids image serialization in the live path. Fixed 1280x720 fixtures measure preprocessing P95 <=20 ms, detection P95 <=40 ms, full pipeline P95 <=70 ms, throughput >=15 FPS, and event latency P95 <=100 ms.

Machine-readable performance and stability JSON drive reports. Stability covers 10,000 frames, 1,000 selections, 500 homographies, 100 source switches, and 100 service start/stop cycles with RSS, audit, state, and Socket checks.
