#!/usr/bin/env python3
import importlib.util
import unittest
from pathlib import Path


SCRIPT_PATH = Path(__file__).resolve().parents[2] / "scripts" / "verify_p1_graphics_backend.py"
SPEC = importlib.util.spec_from_file_location("verify_p1_graphics_backend", SCRIPT_PATH)
MODULE = importlib.util.module_from_spec(SPEC)
assert SPEC and SPEC.loader
SPEC.loader.exec_module(MODULE)


class GraphicsBackendVerificationTests(unittest.TestCase):
    def test_accepts_runtime_reported_metal_rhi(self):
        result = MODULE.classify_log(
            "\n".join(
                (
                    "graphics_api=Metal renderer_interface=RHI rhi_backend=Metal window_title=AstraOS Phone Display",
                    "graphics_api=Metal renderer_interface=RHI rhi_backend=Metal window_title=AstraOS Projection Display",
                )
            )
        )
        self.assertEqual(result, "METAL_CONFIRMED")

    def test_rejects_fallback_backend(self):
        result = MODULE.classify_log(
            "graphics_api=OpenGL renderer_interface=RHI rhi_backend=OpenGL window_title=AstraOS Projection Display"
        )
        self.assertEqual(result, "METAL_FALLBACK_DETECTED")

    def test_rejects_mixed_window_backends(self):
        result = MODULE.classify_log(
            "\n".join(
                (
                    "graphics_api=Metal renderer_interface=RHI rhi_backend=Metal window_title=AstraOS Phone Display",
                    "graphics_api=OpenGL renderer_interface=RHI rhi_backend=OpenGL window_title=AstraOS Projection Display",
                )
            )
        )
        self.assertEqual(result, "METAL_FALLBACK_DETECTED")

    def test_rejects_missing_runtime_report(self):
        self.assertEqual(MODULE.classify_log("QSG_RHI_BACKEND=metal"), "METAL_REQUESTED_BUT_UNCONFIRMED")


if __name__ == "__main__":
    unittest.main()
