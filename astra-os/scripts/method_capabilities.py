#!/usr/bin/env python3
from __future__ import annotations

from functools import lru_cache
from pathlib import Path

import yaml


ROOT = Path(__file__).resolve().parents[1]
REGISTRY = ROOT / "protocols/method-registry.yaml"


@lru_cache(maxsize=1)
def method_capabilities() -> dict[str, str]:
    document = yaml.safe_load(REGISTRY.read_text(encoding="utf-8"))
    methods = document.get("methods", [])
    capabilities = {entry["method"]: entry["capability"] for entry in methods}
    if len(capabilities) != len(methods):
        raise ValueError("method registry contains duplicate methods")
    return capabilities


def capability_for_method(method: str) -> str:
    try:
        return method_capabilities()[method]
    except KeyError as error:
        raise ValueError(f"method has no capability mapping: {method}") from error
