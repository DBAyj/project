# AstraOS UI/UX Guideline v1.0

## Visual and Information Principles

Use dark surfaces, restrained saturation, high contrast, clear depth, and scarce high-emphasis color. The system foregrounds task progress, privacy state, and device status rather than an icon grid. Information hierarchy is task objective, current status, required confirmation, detail, then history.

## Token and Component System

Use semantic tokens from [UI Design Guideline](../architecture/13-ui-design-guideline.md). Baseline components are buttons, inputs, task cards, status cards, projection cards, privacy labels, notifications, modals, loading, error, and empty states. Define component semantic roles, not hardcoded color values, in future Qt design tokens.

## Layout, Typography, Icons, Motion, and Accessibility

Use a consistent spacing scale, compact 8 px-or-less component corners unless a tool surface needs an explicit frame, familiar icons with textual labels only for clear commands, and a redistributable Linux font plan. Every state has keyboard/touch focus, screen-reader name, contrast-compliant color, and non-color status cue. Motion follows 150-250 ms ordinary transitions, 200-350 ms window transitions, and 250-500 ms spatial entry, with reduced-motion fallback.

## Surface-Specific Rules

Phone UI exposes private detail and confirmation. Projection UI shows only policy-approved content, high-distance legibility, target status, and an always-visible privacy indicator. Spatial UI attaches objects to named anchors and must not hide safety state. Error messages state action, context, and recovery; loading and empty states preserve task orientation rather than decorative animation.
