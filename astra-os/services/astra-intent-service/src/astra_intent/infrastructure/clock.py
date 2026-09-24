from __future__ import annotations

from datetime import datetime, timezone


class SystemClock:
    def now(self) -> datetime:
        return datetime.now(timezone.utc)

    def iso_now(self) -> str:
        return self.now().isoformat(timespec="milliseconds").replace("+00:00", "Z")
