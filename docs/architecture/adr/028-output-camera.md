# ADR-028 — Output camera and format-7 scene framing

Status: bounded HM-13 implementation, 2026-09-23.

## Decision

Format 7 stores one explicit `activeCamera` ID and a root `Camera` layer. The camera
owns no drawing or child layers. Its authored rest pose and frame keys use the
existing transform evaluator, undoable document commands and rational frame
index. Earlier formats load without a camera; saving a format-6 project makes
the normal source-version backup before committing format 7. A missing, duplicate,
hidden, solo, parented or singular camera is rejected by document validation.
Camera zoom keys may use only easing handles within their keyed value range, so
interpolation cannot cross zero or invert the projection.

The output mapping is orthographic: translate the evaluated camera center to
the output center, counter-rotate it, then apply its zoom. The render adapter
uses this mapping once when painting scene artwork. Both the legacy painter and
the typed linear compositor share the same mapping and clipping boundary.
Display and Write therefore sample the same camera pose; the camera layer never
contributes pixels. The Qt canvas offers a separate stage view while the Camera
tool is active so framing handles can be dragged directly. Ordinary drawing
tools use the projected view and invert the camera mapping for picking and
drawing-local edits. Viewport fit, pan and zoom remain UI state and never enter
the document or export snapshot.

## Interaction and bounds

The one-camera Camera tool provides frame dragging for pan, a rotation handle,
corner handles for zoom, Shift-constrained pan/rotation, a reset action and a
compact numeric zoom control. The Properties panel exposes the selected camera
pose. A dashed safe-area guide is a nonrendering overlay. Locked cameras reject
edits. Camera creation, key edits, deletion and undo/redo retain selection
validity before UI notifications.

This slice does not add multiplane parallax, perspective, camera switching or
scene-space deformation. `HM13-ACCEPTANCE.md` records preview, reopened output,
native gestures, PNG export and invalid-framing evidence. The wider P06 camera
catalog and all P06 exit criteria remain open.
