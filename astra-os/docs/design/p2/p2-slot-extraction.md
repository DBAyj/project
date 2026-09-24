# P2 Slot Extraction

Slot extraction maps explicit text to `target_space`, `model_id`, `privacy_level`, `zoom_factor`, `rotation_direction`, `rotation_degrees`, `display_target`, and `confirmation_response`. Chinese spatial terms map to desk, wall, projection screen, phone screen, or current space. Privacy values use the five frozen levels. Zoom and rotation values are range-checked.

Defaults are allowed only when declared by a matching rule. Unreliable values remain null. Missing required values force clarification or rejection; they never receive fabricated parameters.
