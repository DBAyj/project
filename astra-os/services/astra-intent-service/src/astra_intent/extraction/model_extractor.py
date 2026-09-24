from __future__ import annotations

import re


class ModelExtractor:
    _known_models = (
        ("demo-device", ("设备模型", "设备三维", "device model", "3d device")),
    )
    _named_model = re.compile(r"(?:切换到|换成|switch to)\s*([\w\u4e00-\u9fff-]{2,32}?)(?:模型|\s+model)", re.IGNORECASE)

    def extract(self, text: str) -> str | None:
        comparable = text.casefold()
        for model_id, phrases in self._known_models:
            if any(phrase in comparable for phrase in phrases):
                return model_id
        match = self._named_model.search(text)
        if match:
            return match.group(1).strip().casefold().replace(" ", "-")
        return None
