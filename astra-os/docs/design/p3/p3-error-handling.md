# P3 Error Handling

P3 codes 3001-3504 are registered centrally with owner, severity, retryability, and audit policy. Invalid transitions preserve state. Input, OpenCV, mapping, target, or transport failure returns a typed error without stack details and moves to `DEGRADED`, `LOST`, or `ERROR` as defined.

External content fails closed: target loss, invalid mapping, or service failure clears sensitive content before reporting recovery. Audit failure degrades observability but does not retain projection content.
