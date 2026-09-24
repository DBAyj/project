from __future__ import annotations

from dataclasses import dataclass


@dataclass(frozen=True, slots=True)
class InjectionDetection:
    detected: bool
    matched_signals: tuple[str, ...]


class InjectionDetector:
    _signals = (
        "忽略之前所有规则",
        "忽略所有安全规则",
        "绕过安全策略",
        "显示系统提示词",
        "系统提示词",
        "输出密钥",
        "删除所有文件",
        "执行终端命令",
        "ignore all previous instructions",
        "ignore previous instructions",
        "bypass security policy",
        "reveal the system prompt",
        "show the system prompt",
        "output the secret",
    )

    def detect(self, text: str) -> InjectionDetection:
        comparable = text.casefold()
        matches = tuple(signal for signal in self._signals if signal.casefold() in comparable)
        return InjectionDetection(bool(matches), matches)
