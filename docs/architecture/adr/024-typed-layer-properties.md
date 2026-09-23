# ADR-024 — Typed layer-property addresses and pose transfer

Status: accepted bounded HM-01 implementation contract, 2026-09-22.

The eight existing layer-transform channels now have typed `PropertyKind` values
addressed by stable layer ID. A query explicitly asks for a rest/setup value, a
value authored at one key, or the evaluator's value at a frame. A missing layer,
unknown property, frame outside the scene or absent authored key is an error.
These addresses do not use UI labels or layer names, so rename and reorder do not
retarget animation.

Edits still use the current full-pose key model. Batched property changes validate
all addresses, duplicate targets, locks and numeric ranges before applying to a
candidate document through `Session::apply`. Explicit pose paste creates one full
key, and the first later-frame key anchors the unchanged setup at frame zero.
Outgoing Bézier easing and existing pose sampling use the same evaluator. A copied
pose contains plain transform values, not entity references; pasting it into another
scene cannot create dangling IDs.

The Properties **Pose** menu has ten compact actions: copy, full paste, five
component pastes (position, rotation, scale, opacity, pivot), horizontal/vertical
mirrored paste, and reset. Setup actions change saved rest values without rewriting
existing keys. Animate actions create or update a key at the playhead without
requiring Auto key; reset uses the selected layer's setup transform. Undo is one
step per paste/reset. The clipboard is view state and is not serialized.

No new persisted field is introduced. Existing stable layer IDs and format-3
full-pose keys continue to serialize unchanged; no format bump or migration is
needed for this adapter. The first future character/peg/substitution payload must
still follow ADR-023's format-4 migration and backup rule. This work does not add
independent channel-key timing, character pose masks, controls or deformer drivers.
