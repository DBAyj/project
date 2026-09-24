from __future__ import annotations


class TargetExtractor:
    _targets = (
        ("projection_screen", ("投影屏", "projection screen")),
        ("phone_screen", ("手机上", "手机屏", "phone screen")),
        ("desk", ("桌面", "桌上", "desk", "desktop")),
        ("wall", ("墙上", "墙面", "wall")),
        ("current_space", ("这里", "当前位置", "current space")),
    )

    def extract(self, text: str) -> str | None:
        comparable = text.casefold()
        for target, phrases in self._targets:
            if any(phrase in comparable for phrase in phrases):
                return target
        return None
