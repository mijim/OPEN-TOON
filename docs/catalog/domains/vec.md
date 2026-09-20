# VEC — Vector drawing and geometry editing

[Back to catalog](../README.md)

> Generated from `features.json` and `domains.json`; do not edit manually.

**Workflow:** Sketch → select → clean up → edit contours → finish drawing.

**Module:** `drawing-vector`.

**Entities:** VectorStroke, BezierPath, WidthProfile, FillRegion, Selection.

**Relationships:** LYR, UI.

**Main risk:** Topology, precision and stroke quality require engine work.

## Shared contract

Mutations must respect transactions, undo/redo and persistence. Visual aids are not exported. Errors, cancellation and unsupported data must preserve the last valid state. These OPEN-TOON conditions will be specified per operation during implementation.

## VEC-001 — Centerline pencil

Represent strokes as curves with editable width and configurable caps.

**Initial acceptance:** Changing width does not move the centerline.

**Scope:** `base` · **Level:** `core` · **Status:** `not_started`.

**Evidence:** `proposal`.

## VEC-002 — Vector brush

Create editable filled silhouettes from drawing gestures.

**Initial acceptance:** A closed stroke remains editable through control points.

**Scope:** `base` · **Level:** `core` · **Status:** `not_started`.

**Evidence:** `proposal`.

## VEC-003 — Variable width

Apply pressure and variable profiles along a stroke.

**Initial acceptance:** Fine stroke ends are preserved when zooming in.

**Scope:** `base` · **Level:** `core` · **Status:** `not_started`.

**Evidence:** `proposal`.

## VEC-004 — Pencil texture

Attach a texture and repetition parameters to a centerline.

**Initial acceptance:** Changing stroke length preserves the defined texture continuity.

**Scope:** `base` · **Level:** `pro` · **Status:** `not_started`.

**Evidence:** `proposal`.

## VEC-005 — Brush presets

Save size, tip, smoothing and texture as a reusable preset.

**Initial acceptance:** Exporting and importing the preset preserves a sample stroke's appearance.

**Scope:** `base` · **Level:** `core` · **Status:** `not_started`.

**Evidence:** `proposal`.

## VEC-006 — Pencil presets

Save line profiles and settings as distinct presets.

**Initial acceptance:** Switching presets does not modify existing strokes.

**Scope:** `base` · **Level:** `core` · **Status:** `not_started`.

**Evidence:** `proposal`.

## VEC-007 — Stroke stabilizer

Smooth pen noise through configurable lag and stabilization.

**Initial acceptance:** The stroke reaches its pen-up endpoint without an unintended segment.

**Scope:** `base` · **Level:** `core` · **Status:** `not_started`.

**Evidence:** `proposal`.

## VEC-008 — Vector eraser

Trim geometry crossed by an eraser with size and pressure control.

**Initial acceptance:** Partially erasing a line leaves two valid segments.

**Scope:** `base` · **Level:** `core` · **Status:** `not_started`.

**Evidence:** `proposal`.

## VEC-009 — Selection and lasso

Select strokes or regions, add to or subtract from the selection, and transform it.

**Initial acceptance:** A partial selection does not move strokes outside the selected area.

**Scope:** `base` · **Level:** `core` · **Status:** `not_started`.

**Evidence:** `proposal`.

## VEC-010 — Contour editor

Add, delete and move curve points and tangents.

**Initial acceptance:** Deleting a point leaves a valid curve, and undo restores it.

**Scope:** `base` · **Level:** `core` · **Status:** `not_started`.

**Evidence:** `proposal`.

## VEC-011 — Centerline editor

Edit a stroke's center path while preserving brush appearance where possible.

**Initial acceptance:** Moving a control point does not change the fill color.

**Scope:** `base` · **Level:** `pro` · **Status:** `not_started`.

**Evidence:** `proposal`.

## VEC-012 — Width editor

Modify a pencil profile without redrawing its path.

**Initial acceptance:** A localized width adjustment does not affect the rest of the stroke.

**Scope:** `base` · **Level:** `pro` · **Status:** `not_started`.

**Evidence:** `proposal`.

## VEC-013 — Pencil retouching

Paint width or opacity adjustments using increase, decrease, replace and smooth modes.

**Initial acceptance:** Smoothing opacity does not move control points.

**Scope:** `base` · **Level:** `pro` · **Status:** `not_started`.

**Evidence:** `proposal`.

## VEC-014 — Post-stroke smoothing

Reduce contour irregularities with adjustable strength.

**Initial acceptance:** The operation preserves protected corners within the defined tolerance.

**Scope:** `base` · **Level:** `pro` · **Status:** `not_started`.

**Evidence:** `proposal`.

## VEC-015 — Lines and primitives

Draw lines, rectangles and ellipses with constraints and optional fills.

**Initial acceptance:** Holding the constraint produces a geometric circle.

**Scope:** `base` · **Level:** `core` · **Status:** `not_started`.

**Evidence:** `proposal`.

## VEC-016 — Polylines and Bézier curves

Construct precise curves using points and handles.

**Initial acceptance:** Closing a path creates a paintable region without a residual gap.

**Scope:** `base` · **Level:** `core` · **Status:** `not_started`.

**Evidence:** `proposal`.

## VEC-017 — Cutter

Separate a region with a lasso and retain it as an editable selection.

**Initial acceptance:** Moving the cutout does not drag the surrounding drawing.

**Scope:** `base` · **Level:** `pro` · **Status:** `not_started`.

**Evidence:** `proposal`.

## VEC-018 — Perspective deformation

Deform a selection using perspective controls.

**Initial acceptance:** Undo restores the previous points exactly.

**Scope:** `base` · **Level:** `pro` · **Status:** `not_started`.

**Evidence:** `proposal`.

## VEC-019 — Drawing envelope

Edit an envelope that deforms selected geometry.

**Initial acceptance:** The envelope affects only the captured selection.

**Scope:** `base` · **Level:** `pro` · **Status:** `not_started`.

**Evidence:** `proposal`.

## VEC-020 — Invisible strokes

Define fill boundaries that produce no visible line.

**Initial acceptance:** The boundary contains the paint and is absent from exports.

**Scope:** `base` · **Level:** `pro` · **Status:** `not_started`.

**Evidence:** `proposal`.

## VEC-021 — Stroke conversion

Convert line types under explicit constraints and retain a recoverable copy.

**Initial acceptance:** The conversion reports any texture or opacity that is not preserved.

**Scope:** `base` · **Level:** `pro` · **Status:** `not_started`.

**Evidence:** `proposal`.

## VEC-022 — Vector optimization

Simplify geometry and combine compatible elements within a configured tolerance.

**Initial acceptance:** Simplification respects a published visual-error tolerance.

**Scope:** `base` · **Level:** `pro` · **Status:** `not_started`.

**Evidence:** `proposal`.

## VEC-023 — Drawing stamps

Stamp reusable drawings with position, scale and rotation.

**Initial acceptance:** Each stamp retains its insertion transform.

**Scope:** `base` · **Level:** `pro` · **Status:** `not_started`.

**Evidence:** `proposal`.

## VEC-024 — Text

Create editable text and convert it to geometry when portability is needed.

**Initial acceptance:** Converted text retains its appearance without the font installed.

**Scope:** `base` · **Level:** `pro` · **Status:** `not_started`.

**Evidence:** `proposal`.

## VEC-025 — Guides and grid

Show a grid and reference guides with optional snapping.

**Initial acceptance:** Disabling snapping permits points between grid lines.

**Scope:** `base` · **Level:** `core` · **Status:** `not_started`.

**Evidence:** `proposal`.

## VEC-026 — Perspective guides

Assist drawing using vanishing points, isometric and curved perspectives.

**Initial acceptance:** Moving a guide changes assistance without modifying finished drawings.

**Scope:** `base` · **Level:** `pro` · **Status:** `not_started`.

**Evidence:** `proposal`.

## VEC-027 — Reposition all drawings

Apply a geometric transform to a set of drawings belonging to an element.

**Initial acceptance:** The action moves exposed and unexposed drawings according to the explicit selection.

**Scope:** `base` · **Level:** `pro` · **Status:** `not_started`.

**Evidence:** `proposal`.
