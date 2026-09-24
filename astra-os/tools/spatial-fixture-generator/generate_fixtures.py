#!/usr/bin/env python3
from __future__ import annotations

import argparse
import binascii
from pathlib import Path
import struct
import zlib


WIDTH = 1280
HEIGHT = 720
Color = tuple[int, int, int]
Point = tuple[int, int]


class Canvas:
    def __init__(self, base: Color = (24, 29, 38)) -> None:
        self.pixels = bytearray(WIDTH * HEIGHT * 3)
        for y in range(HEIGHT):
            shade = (base[0] + y // 48, base[1] + y // 42, base[2] + y // 36)
            row = bytes(shade) * WIDTH
            start = y * WIDTH * 3
            self.pixels[start : start + len(row)] = row

    def pixel(self, x: int, y: int, color: Color) -> None:
        if 0 <= x < WIDTH and 0 <= y < HEIGHT:
            index = (y * WIDTH + x) * 3
            self.pixels[index : index + 3] = bytes(color)

    def line(self, start: Point, end: Point, color: Color, width: int = 3) -> None:
        x0, y0 = start
        x1, y1 = end
        dx = abs(x1 - x0)
        sx = 1 if x0 < x1 else -1
        dy = -abs(y1 - y0)
        sy = 1 if y0 < y1 else -1
        error = dx + dy
        while True:
            for offset_y in range(-width // 2, width // 2 + 1):
                for offset_x in range(-width // 2, width // 2 + 1):
                    self.pixel(x0 + offset_x, y0 + offset_y, color)
            if x0 == x1 and y0 == y1:
                break
            twice = 2 * error
            if twice >= dy:
                error += dy
                x0 += sx
            if twice <= dx:
                error += dx
                y0 += sy

    def polygon(self, points: list[Point], fill: Color, outline: Color = (235, 240, 246)) -> None:
        minimum_y = max(0, min(y for _, y in points))
        maximum_y = min(HEIGHT - 1, max(y for _, y in points))
        for y in range(minimum_y, maximum_y + 1):
            intersections: list[int] = []
            for index, current in enumerate(points):
                following = points[(index + 1) % len(points)]
                x0, y0 = current
                x1, y1 = following
                if y0 == y1 or y < min(y0, y1) or y >= max(y0, y1):
                    continue
                intersections.append(round(x0 + (y - y0) * (x1 - x0) / (y1 - y0)))
            intersections.sort()
            for start, end in zip(intersections[0::2], intersections[1::2]):
                start = max(0, start)
                end = min(WIDTH - 1, end)
                if start <= end:
                    offset = (y * WIDTH + start) * 3
                    self.pixels[offset : offset + (end - start + 1) * 3] = bytes(fill) * (end - start + 1)
        for index, point in enumerate(points):
            self.line(point, points[(index + 1) % len(points)], outline, 4)

    def rectangle(self, left: int, top: int, right: int, bottom: int, color: Color) -> None:
        for y in range(max(0, top), min(HEIGHT, bottom)):
            start = (y * WIDTH + max(0, left)) * 3
            count = max(0, min(WIDTH, right) - max(0, left))
            self.pixels[start : start + count * 3] = bytes(color) * count

    def grid(self, spacing: int = 80) -> None:
        for x in range(0, WIDTH, spacing):
            self.line((x, 0), (x, HEIGHT - 1), (92, 112, 139), 1)
        for y in range(0, HEIGHT, spacing):
            self.line((0, y), (WIDTH - 1, y), (92, 112, 139), 1)


def png_chunk(kind: bytes, payload: bytes) -> bytes:
    return struct.pack(">I", len(payload)) + kind + payload + struct.pack(">I", binascii.crc32(kind + payload) & 0xFFFFFFFF)


def write_png(path: Path, canvas: Canvas) -> None:
    rows = bytearray()
    stride = WIDTH * 3
    for y in range(HEIGHT):
        rows.append(0)
        rows.extend(canvas.pixels[y * stride : (y + 1) * stride])
    header = struct.pack(">IIBBBBB", WIDTH, HEIGHT, 8, 2, 0, 0, 0)
    value = b"\x89PNG\r\n\x1a\n" + png_chunk(b"IHDR", header) + png_chunk(b"IDAT", zlib.compress(rows, 9)) + png_chunk(b"IEND", b"")
    path.write_bytes(value)


def fixtures() -> dict[str, Canvas]:
    values: dict[str, Canvas] = {}

    canvas = Canvas()
    canvas.polygon([(160, 260), (1120, 260), (1080, 650), (200, 650)], (50, 112, 126))
    values["desk-front.png"] = canvas

    canvas = Canvas()
    canvas.polygon([(240, 220), (1080, 300), (990, 660), (120, 570)], (48, 117, 132))
    values["desk-oblique.png"] = canvas

    canvas = Canvas((31, 31, 42))
    canvas.polygon([(190, 90), (1090, 90), (1090, 620), (190, 620)], (93, 98, 117))
    values["wall-front.png"] = canvas

    canvas = Canvas((31, 31, 42))
    canvas.polygon([(260, 90), (1110, 170), (1020, 620), (160, 560)], (91, 100, 121))
    values["wall-oblique.png"] = canvas

    canvas = Canvas()
    canvas.polygon([(70, 120), (590, 100), (620, 400), (90, 430)], (95, 101, 120))
    canvas.polygon([(670, 300), (1190, 260), (1130, 670), (620, 650)], (45, 120, 132))
    values["multiple-surfaces.png"] = canvas

    canvas = Canvas((5, 7, 10))
    canvas.polygon([(180, 260), (1090, 240), (1060, 640), (210, 660)], (18, 31, 35), (68, 75, 82))
    values["low-light.png"] = canvas

    canvas = Canvas()
    canvas.polygon([(170, 250), (1110, 240), (1080, 650), (200, 660)], (51, 112, 126))
    canvas.rectangle(530, 330, 820, 720, (16, 18, 24))
    values["partial-occlusion.png"] = canvas

    values["no-surface.png"] = Canvas((28, 33, 43))

    canvas = Canvas()
    canvas.polygon([(600, 330), (680, 330), (680, 380), (600, 380)], (50, 112, 126))
    values["invalid-small-surface.png"] = canvas

    canvas = Canvas((18, 22, 30))
    canvas.grid()
    canvas.polygon([(180, 100), (1100, 100), (1100, 650), (180, 650)], (35, 42, 54), (232, 238, 245))
    canvas.grid(80)
    values["calibration-grid.png"] = canvas
    return values


def generate(output: Path) -> None:
    output.mkdir(parents=True, exist_ok=True)
    for name, canvas in fixtures().items():
        write_png(output / name, canvas)


def main() -> int:
    parser = argparse.ArgumentParser(description="Generate deterministic AstraOS P3 spatial fixtures")
    parser.add_argument("--output", type=Path, required=True)
    arguments = parser.parse_args()
    generate(arguments.output)
    print(f"generated={len(fixtures())} output={arguments.output}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
