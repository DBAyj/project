# P2 Candidate Merging

Candidates are grouped by intent. Rule and model scores use configurable weights. Agreement adds a bonus; disagreement applies a conflict penalty; compatible current context adds a bonus; missing required slots apply a penalty. Scores are clamped to `[0, 1]`, sorted by score then stable intent name, and retained for explanation.

An exact high-priority rule remains the leading source. A top-two delta below the configured ambiguity threshold prevents automatic execution. Fusion never invents a candidate or a slot that no source/extractor supplied.
