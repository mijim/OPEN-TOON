# ADR-019 — Canvas motion-path position editing

Accepted for the experimental P06-W2 subset, 2026-09-20. Feature ANI-008 remains
partial: this is editing pose positions on an evaluated trajectory, not an independent
spatial spline with a separate progress/velocity function.

## Reference point and evaluation

Animate's compact **Path** toggle captures the center of the current drawing's local
bounds. This local reference stays fixed while scrubbing, dragging and undoing, even when a
different drawing is exposed at another frame. Changing the selected layer, replacing the scene or
re-entering Path captures a new reference; it is transient view state, not project data.
The overlay evaluates that reference through the same local/parent transform hierarchy
as the renderer at each sampled frame. Animated ancestors, rotation, pivots, negative
scales and nonuniform scales therefore appear in the visible trajectory.

Sampling includes a bounded regular grid plus every pose key and its preceding frame.
The displayed polyline remains an integer-frame approximation. Markers represent the
selected layer's pose keys; they do not represent parent keys or an editable vector
contour. The current frame is labelled; coincident markers prefer the current key.
Timeline/Curves key navigation can disambiguate overlapping held poses.

## Direct interaction

Click a marker to select its frame. Drag it with an eleven-screen-pixel hit margin to
move that pose's X/Y. The press offset is retained, so selecting beside a marker does
not make it jump. Screen displacement is mapped through the inverse **parent** world
transform evaluated at that key, including the viewport transform. The child's own
rotation/scale do not participate in this inverse. A collapsed child can therefore be
moved; a singular parent rejects the gesture. Shift locks the dominant screen axis.
Only X/Y change: timing, rotation, scale, opacity, pivots, base interpolation, outgoing
easing, drawing geometry, exposures, parent keys and other poses remain intact.

A copied document supplies live artwork/trajectory preview without mutating the
Session. Release commits one command; Escape, ungrab, frame/document changes, view
changes and canvas resizing cancel it. Locked layers and playback reject input.
Path mode hides the pose transform box to avoid competing handles; switching it off
restores normal move/scale/rotate gestures.

Click a sampled path segment to scrub. Double-click records the evaluated full pose
at the nearest sampled integer frame. Actual evaluated screen distance is checked,
so the guide across an instantaneous held jump cannot create a phantom intermediate
position. Existing keys are preserved. Inserting a new key uses the existing Add key
semantics: its outgoing interpolation starts linear, and the preceding normalized
curve now ends at that new pose. This can change interpolation between keys; it is
not exact Bezier subdivision or a geometry-preserving change of velocity.

## Ownership and verification

- `modules/document/animation`: validated atomic X/Y edits of existing keys.
- Qt animation adapter: Session command and undo/redo.
- `canvas_motion_path`: reference, hierarchy mapping, hit testing and preview.
- QML workspace: English compact Path toggle and interaction hint.

Domain/controller tests check non-position preservation, bounds, invalid/locked keys,
undo, saving and pixel equivalence. Native mouse checks exercise an animated rotated
parent with negative/nonuniform scales, screen-space picking/grab offset, mirrored and
rotated view, Shift constraint, cancellation, insertion, zero-scale guards and persistence.
No new dependency or document-format change. Physical tablet qualification, dense-path
performance, parent-key manipulation, spatial tangents and separate velocity remain open.
