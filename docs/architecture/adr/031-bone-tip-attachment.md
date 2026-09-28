# ADR-031 — Bounded bone-tip attachment

Status: working HM-06 subset, 2026-09-29. This moves one evaluated-endpoint
link forward from the wider HM-09 rig contract in response to the owner's
continuous-character requirement. IK, arbitrary deformer outputs, rig recipes
and portable templates remain HM-09 work.

## Contract

Format 10 adds `followParentBoneTip` to a Part. A linked Part must be a direct
child of a Part whose exposed drawing has a two-segment bone binding at every
frame. Every exposed source substitution must have its own binding. The source
must evaluate to rest at frame zero, so enabling a link preserves the assembled
rest image. Existing follower transform keys must be cleared before the link is
made; later keys are offsets in the moving tip space. The link stores no second
source ID: the validated parent identity is authoritative.

Evaluation samples the active source substitution's bone chain at the scene
frame. It maps the source-local rest tip to the source-local evaluated tip and
rotates by the sum of shoulder and elbow angles. A child's own transform is
applied first, followed by that tip delta and then the parent's world transform.
Descendants inherit the same mapping. The follower is transformed once; its
pixels are not passed through the parent's mesh. Canvas picking, handles,
preview and output all use the renderer's world-transform function, while the
domain provides the evaluated joint coordinates without Qt dependencies.

The editor exposes the link under Rig after the child Part is parented to a
bone-bound Part. Enabling and disabling it uses the ordinary document command
path with undo/redo. An incomplete source exposure, missing bone on a selected
variant, non-rest source at frame zero or unsupported reparent rejects before
publication. Removing a required source bone or exposing an unbound variant is
also rejected by document validation. A linked child must be unlinked before
reparenting or detaching it.

Formats 1–9 load with the link disabled. The first save of a format-9 project
preserves a `.pre-v9.bak` source copy before writing format 10. The 15-artwork
Part fixture now uses four saved links rather than duplicated hand/foot
position keys. Native interaction, per-frame endpoint, variant, recovery and
render checks are recorded in `docs/implementation/HM06-PROGRESS.md`.

## Limits

This profile links only Part children to the tip of a direct parent two-segment
bone. It requires source bone coverage throughout the scene and does not
provide curve attachments, arbitrary pins, constraint switching, IK or
automatic remapping between unrelated rigs. Full-shot performance and artistic
acceptance remain open.
