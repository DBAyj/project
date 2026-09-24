# ADR-0020: Apply Privacy Filtering Before P4 Submission

- Status: Accepted for P5 fixture-mode development
- Date: 2026-07-15

Policy-owned privacy levels are evaluated before a component can enter the P4
layer list. P4's final privacy mask remains mandatory defense in depth. Privacy
changes synchronously remove unsafe layers and cached UI.
