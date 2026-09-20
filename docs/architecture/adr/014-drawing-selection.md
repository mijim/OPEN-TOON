# ADR-014 — Rectangular mixed-media drawing selection

Interaction and free-transform scope superseded by [ADR-015](015-direct-editing-workspace.md). The following records the experimental.3 contract.

Accepted for experimental.3, 2026-09-20. Owners: Qt-free
`modules/document/drawing_selection` and the canvas/application adapters. Existing
MyPaint and immutable raster tiles are reused; no new dependency or format change.

A selection is a half-open integer rectangle in the selected layer's drawing-local
coordinates. The canvas inverts the same hierarchy/view transform used for painting.
It rounds outward to pixel boundaries; movement snaps to whole drawing-local pixels.
Selection/view state is not persisted, animated or exported. Changing frame, layer,
tool or document invalidates it; a successful region command restores its new bounds.

Media filters are Vectors, Raster pixels, or Both. Vector selection includes only
whole strokes whose point bounds plus half their width fit inside the rectangle.
It applies across the four art layers. It is deliberately conservative for pressure
strokes; it does not split crossing contours, perform region topology, or select by
visible fill intersection. Selected strokes keep shape, point pressure, art-layer
and swatch identity; duplicates allocate new stroke IDs. Imported `ImageAsset` media
are excluded and remain unchanged. Raster operations affect the MyPaint raster only.

Move and duplicate use integer offsets; flips reflect about the selection center.
Clockwise 90-degree rotation exchanges width/height and keeps the top-left corner
fixed. Vector coordinates rotate continuously about that rectangular mapping; pixel
indices rotate about pixel centers. Repeated quarter turns round-trip without
resampling. Free rotation, scaling, soft masks, lasso and additive/subtractive
selection are not implemented.

Raster operations read from the original immutable snapshot, clear source pixels
for a move/flip/rotation/delete, then composite selected pixels source-over the
remaining destination. Transparent selected pixels do not erase destination pixels.
Overlapping moves therefore cannot recursively smear already-moved pixels. Alpha
uses the existing 15-bit premultiplied RGBA convention with integer rounding.
Unchanged tiles stay shared; touched tiles copy once and empty tiles are removed.
A 4096-tile scratch limit bounds mutable tile storage to 128 MiB, excluding input and
published snapshots. Out-of-canvas nontransparent pixels reject the entire operation
rather than silently clipping. This is synchronous and not yet a production large-
selection latency guarantee. Commands validate and publish through `Session::apply`.

The drag previews bounds and selected vector bounds; artwork changes once at release.
Escape cancels the active gesture. Numeric Move/Duplicate, Delete, Flip and Rotate
controls share the same operations. Selection edits affect every exposure referencing
the same drawing; duplicate the drawing first when independent artwork is required.

Raster brush opacity is session tool state in [0,1] passed to MyPaint. Painting also
uses selected swatch alpha. Erasing uses brush opacity independently of swatch alpha,
so selecting a transparent swatch does not disable the eraser.
