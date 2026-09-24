# 13 UI Design Guideline

The visual direction is dark, low-saturation, high-contrast, and spatially legible. Task context takes precedence over a traditional grid of app icons. Components consume semantic tokens only: `surface.primary`, `surface.secondary`, `surface.elevated`, `text.primary`, `text.secondary`, `accent.primary`, `accent.ai`, `accent.projection`, `status.success`, `status.warning`, `status.error`, `privacy.public`, `privacy.restricted`, and `privacy.private`.

The component baseline is button, input, task card, status card, projection card, privacy label, system notification, modal, loading state, error state, and empty state. Phone UI keeps private detail compact; projection UI prioritizes legibility and always exposes its privacy state; spatial UI remains anchored to explicit scene objects.

Normal transitions are 150-250 ms, windows 200-350 ms, and spatial entries 250-500 ms. Reduced-motion mode substitutes immediate or minimal transitions. macOS may use a system font during development, while Linux product builds must use a redistributable font. Detailed guidance is [UI/UX Guideline](../ui/AstraOS-UI-UX-Guideline-v1.0.md).
