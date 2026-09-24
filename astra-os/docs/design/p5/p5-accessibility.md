# P5 Accessibility

Interactive nodes require role, non-empty name, description, state, value,
actions, and focusability. Core work is keyboard-operable with visible focus.
Privacy and error state use text semantics in addition to color. Hidden and
destroyed components are removed from the semantic tree. Reduce-motion and
high-contrast settings are runtime inputs.

The service status publishes both preferences, `SpatialUIStateModel` preserves
them, and `SpatialWorkspace` applies them to QML. Reduce Motion sets spatial
transition duration to zero. High Contrast selects black/white surfaces and a
yellow focus outline, so neither preference is evidence-only metadata.
