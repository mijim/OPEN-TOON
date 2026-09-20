# ADR-016 — Visual pose animation, channel easing and screen-space picking

Status: accepted for the experimental P04/P06 subset, 2026-09-20.

## Interaction and ownership

`Animate` (A) targets the selected drawing layer. Its canvas box moves, scales and
rotates the evaluated pose at the current frame. A completed gesture records one
full-pose key and one undo command; cancellation publishes nothing. The first
later-frame gesture on an unkeyed layer anchors the existing rest pose at frame 0.
The dedicated tool records keys explicitly, independently of the inspector's Auto
key checkbox. Selecting it switches the inspector to Animate mode. `Select` (V)
continues to edit drawing geometry. Shared drawings are never baked by pose edits.

Preview owns a temporary document evaluated by the same renderer as playback and
export. Translation is converted through the original local transform; scale and
rotation preserve their opposite anchor/center in parent coordinates. This avoids
introducing unrepresentable skew under nonuniform scale. Parent transforms still
compose through the renderer. Zero-scale transforms cannot be manipulated by
inverse mapping. The dashed trajectory shows the current drawing box center;
it is a visualization, not a separate velocity-controlled spatial path.

Exposures remain independent. A blank frame has no artwork box; extending the
exposure is an explicit timeline operation. Empty pegs do not have canvas handles.

## Curve contract

A key may store an outgoing `easing` map keyed by one of the eight transform channel
names. Each value has normalized cubic Bézier control points `(x1,y1,x2,y2)`, with
endpoints `(0,0)` and `(1,1)`. Time handles satisfy `0 <= x1 <= x2 <= 1`; value
handles are finite and bounded to `[-4,4]`. Evaluation inverts the cubic time axis
by bounded binary search and interpolates channel values with its vertical result.
This allows overshoot without moving endpoints. Opacity is clamped to `[0,1]`.

A channel override takes precedence over the key's legacy Linear/Hold/Smooth base.
Absent overrides preserve historical behavior. Changing base interpolation clears
that key's overrides; moving a key or editing its value preserves them. Adding a
key at an occupied frame preserves the existing key. Retiming copies normalized
curves with keys. All channels still share full-pose key times.

The integrated graph uses one compact toolbar and an optional Values row. Shared
compact controls also style the rest of the workspace. A visible separator uses
screen coordinates for dragging and fixes the lower panel height within dynamic
window/canvas limits; native checks cover expansion, shrinking and minimum window size.
The integrated graph exposes round handles, live curve preview, double-click key
creation and Linear/Ease in-out/Overshoot presets. Dragging a key changes its time
and selected channel value; collisions are rejected transactionally. Equal endpoint
values have no normalized amplitude, so their handles are hidden: add a middle key
to create an excursion or bounce. This is not a physics bounce generator. Graphs
still fit the whole scene; independent graph zoom and tangent linking remain open.

## Format and migration

Document and SQLite schema version are now **3**. Version 1/2 files remain readable.
The first save of an older schema creates a unique `.pre-v3.bak` SQLite backup
before changing the schema. Existing backups are never overwritten. Old editors
reject schema 3 instead of silently discarding easing. Resource encoding is unchanged
from format 2; compaction requires the current schema. Older revision payloads in a
migrated database remain readable by this version. Unknown channel names, malformed
arrays, unordered handle times and unbounded values are rejected on load.

## Picking and feedback

The Qt render adapter tests vector ink in logical screen coordinates, including a
constant eight-pixel margin, pressure width, fills and art-layer paint order. This
keeps thin strokes selectable at different view/layer scales, rotation, mirroring
and display pixel ratios. It does not enlarge rendered strokes or document geometry.
Hollow shape centers remain unselected outside the margin. Raster picking and
cross-layer object picking are not added by this change.

Tool cursors use original monochrome QPainter artwork. Hover changes to move,
orientation-aware resize, rotate, point-edit or unavailable feedback. Opposing middle
handles are suppressed when they overlap the move target of thin artwork. Point-edit
previews render the entire modified stroke with screen-sized control markers.

## Verification and boundaries

Owning modules: `modules/document` (pose/easing evaluation), `adapters/storage`
(migration), `adapters/render` (picking), `adapters/qt` and `ui/components` (gestures).
Core tests exercise endpoints, overshoot, channel isolation, opacity, rejection,
retiming, undo and persistence. Render tests cover screen-space picking and reopened
pixel equivalence. The native Mac smoke exercises real mouse gestures, cancel,
move/scale/rotation keys, thin-line hover/picking and graph handle edits.

Morphing between different drawings is a distinct P14 capability. Drawing contour
Bézier geometry, deformers, spatial path/velocity separation, independent channel
key times, cameras and large-scene performance remain unimplemented. No phase is
complete. See the [Harmony interaction research](../../research/04-visual-animation.md).
