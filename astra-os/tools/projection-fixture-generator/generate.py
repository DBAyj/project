#!/usr/bin/env python3
"""Generate deterministic P4 visual fixtures without external assets."""

from __future__ import annotations

import argparse
import json
import struct
import zlib
from pathlib import Path


MARKER = "P3_REAL_SPATIAL_INTEGRATION_NOT_VERIFIED"
WIDTH = 256
HEIGHT = 144


def write_png(path: Path, pixels: list[list[tuple[int, int, int, int]]]) -> None:
    height = len(pixels)
    width = len(pixels[0])
    raw = b"".join(
        b"\x00" + bytes(channel for pixel in row for channel in pixel)
        for row in pixels
    )

    def chunk(kind: bytes, data: bytes) -> bytes:
        return struct.pack(">I", len(data)) + kind + data + struct.pack(">I", zlib.crc32(kind + data) & 0xFFFFFFFF)

    content = b"\x89PNG\r\n\x1a\n"
    content += chunk(b"IHDR", struct.pack(">IIBBBBB", width, height, 8, 6, 0, 0, 0))
    content += chunk(b"IDAT", zlib.compress(raw, level=9))
    content += chunk(b"IEND", b"")
    path.write_bytes(content)


def canvas(fill: tuple[int, int, int, int] = (0, 0, 0, 255)) -> list[list[tuple[int, int, int, int]]]:
    return [[fill for _ in range(WIDTH)] for _ in range(HEIGHT)]


def checkerboard() -> list[list[tuple[int, int, int, int]]]:
    return [[(240, 240, 240, 255) if ((x // 16) + (y // 16)) % 2 == 0 else (24, 24, 24, 255) for x in range(WIDTH)] for y in range(HEIGHT)]


def color_bars() -> list[list[tuple[int, int, int, int]]]:
    colors = [(255, 255, 255, 255), (255, 255, 0, 255), (0, 255, 255, 255), (0, 255, 0, 255), (255, 0, 255, 255), (255, 0, 0, 255), (0, 0, 255, 255), (0, 0, 0, 255)]
    return [[colors[min(len(colors) - 1, x * len(colors) // WIDTH)] for x in range(WIDTH)] for _ in range(HEIGHT)]


def grayscale_ramp() -> list[list[tuple[int, int, int, int]]]:
    return [[(x, x, x, 255) for x in range(WIDTH)] for _ in range(HEIGHT)]


def geometry_grid() -> list[list[tuple[int, int, int, int]]]:
    image = canvas((16, 28, 48, 255))
    for y in range(HEIGHT):
        for x in range(WIDTH):
            if x % 16 == 0 or y % 16 == 0:
                image[y][x] = (255, 255, 255, 255)
            elif x == WIDTH // 2 or y == HEIGHT // 2:
                image[y][x] = (255, 80, 80, 255)
    return image


def privacy_mask() -> list[list[tuple[int, int, int, int]]]:
    image = checkerboard()
    for y in range(40, 104):
        for x in range(72, 184):
            image[y][x] = (0, 0, 0, 255)
    return image


def layer_composition() -> list[list[tuple[int, int, int, int]]]:
    image = canvas((24, 48, 144, 255))
    for y in range(28, 116):
        for x in range(44, 212):
            image[y][x] = (40, 192, 128, 255)
    for y in range(52, 92):
        for x in range(88, 168):
            image[y][x] = (255, 192, 40, 255)
    return image


def profile(fixture_id: str, corners: list[tuple[float, float]]) -> dict[str, object]:
    return {
        "version": "1.0",
        "fixture_id": fixture_id,
        "resolution": {"width": WIDTH, "height": HEIGHT},
        "corners": [{"x": x, "y": y} for x, y in corners],
        "p3_integration_status": MARKER,
    }


def generate(destination: Path) -> None:
    destination.mkdir(parents=True, exist_ok=True)
    images = {
        "checkerboard.png": checkerboard(),
        "color-bars.png": color_bars(),
        "grayscale-ramp.png": grayscale_ramp(),
        "geometry-grid.png": geometry_grid(),
        "privacy-mask-test.png": privacy_mask(),
        "layer-composition-test.png": layer_composition(),
    }
    for name, image in images.items():
        write_png(destination / name, image)

    profiles = {
        "homography-front.json": profile("front-rectangle", [(0, 0), (255, 0), (255, 143), (0, 143)]),
        "homography-left-keystone.json": profile("left-keystone", [(28, 0), (228, 0), (255, 143), (0, 143)]),
        "homography-right-keystone.json": profile("right-keystone", [(0, 0), (228, 0), (255, 143), (28, 143)]),
        "homography-top-keystone.json": profile("top-keystone", [(0, 20), (255, 20), (255, 143), (0, 143)]),
        "homography-bottom-keystone.json": profile("bottom-keystone", [(0, 0), (255, 0), (228, 123), (28, 123)]),
        "invalid-homography.json": profile("invalid-self-intersection", [(0, 0), (255, 143), (0, 143), (255, 0)]),
    }
    for name, content in profiles.items():
        (destination / name).write_text(json.dumps(content, indent=2, sort_keys=True) + "\n", encoding="utf-8")


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--output", type=Path, required=True)
    arguments = parser.parse_args()
    generate(arguments.output)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
