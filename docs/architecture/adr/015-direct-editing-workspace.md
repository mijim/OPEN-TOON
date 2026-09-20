# ADR-015 — Direct manipulation and selection-owned Properties

Accepted by owner direction, 2026-09-20. Supersedes the dialog interaction and
bounds-only preview portions of ADR-013/014. No format or dependency change.

Selection transforms operate on the canvas. Eight screen-sized handles scale edges
or corners, a top circle rotates, and dragging the interior moves. Shift constrains
corner aspect ratio or snaps rotation to 15 degrees. The adapter evaluates a private
drawing preview from the original gesture snapshot, renders through the shared scene
renderer, and commits once on release. Escape, focus loss and invalidation discard the
preview. Invalid transforms are rejected as one unit. Undo restores the whole gesture.

Vector selection freezes stroke IDs, so overlapping unrelated objects cannot enter a
transform during a drag. Translation/scaling preserve vector geometry. Rotating a
rectangle converts it to a four-point polygon; rotating an ellipse converts it to a
128-point editable polygon (approximation, not analytic Bezier geometry). Stroke width
scales by the square root of absolute determinant; nonuniform outline deformation is
not exact. Pivot is the opposite handle for scaling and rectangle center for rotation.
The box returns to axis-aligned bounds after committing. Coordinates retain drawing-
local units; viewport zoom, rotation/mirroring and layer transforms affect presentation.

Raster free transforms use inverse pixel-center nearest-neighbor sampling in the
existing premultiplied 15-bit space. Transparent surroundings remain transparent;
nontransparent destination pixels composite source-over after source removal. No
bilinear/smooth interpolation, arbitrary masks or lossless free rotation is claimed.
The output work area is capped at 16 million pixels; source clearing and destination
editing each use the existing 4096-tile cap. These synchronous experimental limits
bound work but are not a production responsiveness guarantee for large selections.

Properties has an explicit target. Selecting a canvas stroke/region shows its bounds
and editable fields; a single vector additionally exposes stroke width, fill, art layer
and swatch. Rotate-by is a relative geometry edit, not stored object transform metadata.
Selecting a layer row/name shows its rest/animated layer transforms, hierarchy and keys.
No canvas object/layer selection shows guidance. Object commands preserve selection
identity; unrelated document/frame changes clear it. Numeric fields rebind after edits,
including rejected edits, instead of retaining stale typed text.

Curves is a tab in the resizable bottom workspace. Timeline/Xsheet timing controls stay
in the workspace, with a range-end drag handle for stretching and Alt-drag for moving.
The three separate selection/timing/curve dialogs are removed. Ordinary file/settings
and confirmation dialogs are unaffected. UI and help remain English.
