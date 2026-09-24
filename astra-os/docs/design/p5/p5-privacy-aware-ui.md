# P5 Privacy-Aware UI

Policy-owned privacy values are evaluated for the target before layer mapping.
PRIVATE_SCREEN_ONLY is phone-only, NO_PROJECTION is excluded, and an
unauthorized AUTHORIZED_PERSON component is hidden. Changes re-evaluate the
entire tree synchronously. This filter supplements, never replaces, P4's final
privacy mask and safe-clear path.

The runtime calls `astra-policy` once per visible candidate layer and maps only
an allowed decision. Each accepted P4 layer carries the policy decision UUID,
so the layer can be correlated with policy and audit evidence without allowing
the Shell, QML, or a process flag to self-authorize projection.

In fixture mode, the validated policy adapter owns a fixed public subject ID;
all other subjects default to `PRIVATE_SCREEN_ONLY`. A request may ask for a
stricter level but cannot downgrade its existing or configured classification.
