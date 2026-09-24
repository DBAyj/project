#!/usr/bin/env python3
from __future__ import annotations

import importlib.util
import unittest
from pathlib import Path


SCRIPT = Path(__file__).resolve().parents[2] / "scripts" / "verify_p4_graphics.py"
SPEC = importlib.util.spec_from_file_location("verify_p4_graphics", SCRIPT)
MODULE = importlib.util.module_from_spec(SPEC)
assert SPEC and SPEC.loader
SPEC.loader.exec_module(MODULE)


class P4GraphicsEvidenceTests(unittest.TestCase):
    def test_accepts_runtime_metal_with_visible_p4_output_frame(self) -> None:
        result = MODULE.classify_log(
            "\n".join(
                (
                    "graphics_api=Metal renderer_interface=RHI rhi_backend=Metal window_title=AstraOS Phone Display",
                    "graphics_api=Metal renderer_interface=RHI rhi_backend=Metal window_title=AstraOS Projection Display",
                    "p4_projection_output_first_frame=true window_visible=true window_title=AstraOS Projection Display p3_integration_status=P3_REAL_SPATIAL_INTEGRATION_NOT_VERIFIED",
                )
            )
        )
        self.assertEqual(result, "P4_METAL_CONFIRMED")

    def test_rejects_requested_metal_without_p4_first_frame(self) -> None:
        self.assertEqual(
            MODULE.classify_log("graphics_api=Metal renderer_interface=RHI rhi_backend=Metal window_title=AstraOS Projection Display"),
            "P4_METAL_REQUESTED_BUT_UNCONFIRMED",
        )

    def test_rejects_runtime_backend_fallback(self) -> None:
        self.assertEqual(
            MODULE.classify_log("graphics_api=OpenGL renderer_interface=RHI rhi_backend=OpenGL window_title=AstraOS Projection Display"),
            "P4_METAL_FALLBACK_DETECTED",
        )


if __name__ == "__main__":
    unittest.main()
