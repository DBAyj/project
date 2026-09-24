from __future__ import annotations

import unittest

from astra_intent.extraction.slot_extractor import SlotExtractor


class SlotExtractorTests(unittest.TestCase):
    def setUp(self) -> None:
        self.extractor = SlotExtractor()

    def test_extracts_target_model_privacy_and_display_target(self) -> None:
        result = self.extractor.extract("把设备模型投到墙上，公开展示")

        self.assertEqual(result.slots.target_space, "wall")
        self.assertEqual(result.slots.model_id, "demo-device")
        self.assertEqual(result.slots.privacy_level, "PUBLIC")
        self.assertEqual(result.slots.display_target, "projection_screen")

    def test_extracts_zoom_rotation_direction_and_degrees(self) -> None:
        result = self.extractor.extract("把模型放大两倍，再向左旋转90度")

        self.assertEqual(result.slots.zoom_factor, 2.0)
        self.assertEqual(result.slots.rotation_direction, "left")
        self.assertEqual(result.slots.rotation_degrees, 90.0)
        self.assertEqual(self.extractor.extract("缩小到一半").slots.zoom_factor, 0.5)

    def test_extracts_phone_only_and_confirmation_responses(self) -> None:
        result = self.extractor.extract("只在手机上显示")

        self.assertEqual(result.slots.target_space, "phone_screen")
        self.assertEqual(result.slots.display_target, "phone_screen")
        self.assertEqual(result.slots.privacy_level, "PRIVATE_SCREEN_ONLY")
        self.assertEqual(self.extractor.extract("确认继续").slots.confirmation_response, "confirm")
        self.assertEqual(self.extractor.extract("拒绝执行").slots.confirmation_response, "reject")

    def test_reports_conflicting_privacy_without_hiding_the_conflict(self) -> None:
        result = self.extractor.extract("给大家公开展示，只在手机上显示")

        self.assertEqual(result.slots.privacy_level, None)
        self.assertEqual(set(result.conflicts), {"PUBLIC", "PRIVATE_SCREEN_ONLY"})

    def test_applies_only_explicit_defaults_and_does_not_invent_slots(self) -> None:
        unresolved = self.extractor.extract("把那个放到那里")
        with_defaults = self.extractor.extract("显示模型", defaults={"model_id": "demo-device", "target_space": "desk"})

        self.assertIsNone(unresolved.slots.model_id)
        self.assertIsNone(unresolved.slots.target_space)
        self.assertEqual(with_defaults.slots.model_id, "demo-device")
        self.assertEqual(with_defaults.slots.target_space, "desk")


if __name__ == "__main__":
    unittest.main()
