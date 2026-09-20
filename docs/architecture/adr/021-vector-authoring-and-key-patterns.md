# ADR-021 — Vector authoring and pose-key patterns

Status: accepted for the experimental subset, 2026-09-21. Extends ADR-020 and ADR-018.

## Decision

Keep geometry and key operations in Qt-free document modules. The Qt adapter owns
selection, clipboard, snapping and view state. Every document edit validates a
candidate and publishes through one undoable Session transaction. No new dependency
or file-format change is needed; Qt's existing path implementation handles lasso
containment in the render adapter.

## Selection and composition

- Lasso selects complete visible vector footprints, including pressure width,
  caps and fills, inside an odd-even closed polygon. Crossing strokes are excluded.
  Concave lassos cannot select strokes spanning an excluded area. Sampling is bounded
  at 4,096 points. Shift adds and Alt subtracts; Escape cancels.
- Select all/invert operates on vectors in the active drawing. Selection membership
  remains explicit; interior bystanders are not implicitly included in edits.
- The application-local vector clipboard preserves original stacking order, geometry,
  pressure and art layers across drawings and scenes. Paste creates new identities;
  swatches are reused only when name and RGBA match, otherwise new swatches are created.
  Locked or invalid destinations reject the entire paste. This is not OS clipboard,
  SVG interchange, raster copy or world-space retargeting.
- Align uses outward-rounded local stroke bounds. Distribute requires three objects,
  evenly spaces their centers and preserves outer anchors. Nudge is in drawing units.
- Bring/send operations are stable within each art layer; they never cross art layers.
  Applying an art-layer style explicitly changes that membership.
- Group width, swatch, fill and art-layer edits retain identity and geometry. Invalid
  values or swatches reject atomically. Mixed widths are shown as Mixed in Properties.

## Drawing and cleanup

Line creates a two-point sampled stroke. Shift rounds its angle to 45 degrees;
Shift Rectangle/Ellipse creates equal dimensions. Drawing-local grid snapping applies
to Pencil/Line/Rectangle/Ellipse before shape constraints; constraints take priority.
The adaptive grid is a view overlay, excluded from documents and exports. Spacing is
bounded to 2–1,000 units and display to 512 lines per axis; preferences are session-only.

Smoothing and simplification apply only to sampled open pencil strokes. Smoothing
uses a chosen strength, retains endpoints and pressure, and protects corners of at
least 60 degrees. Simplification retains original points using iterative RDP with
local-coordinate tolerance, plus pressure error no greater than 0.02. It is bounded
to five million point-segment evaluations and rejects atomically on exhaustion.
These are centerline/pressure constraints, not guarantees of identical rendered pixels
or replacements for analytic contour, variable-width or topology editors.

## Pose-key patterns

Batch interpolation edits selected full-pose keys on one layer. Linear/Step/Smooth
clear stale explicit easing; ease/overshoot/fast-start presets write all eight outgoing
channel curves. Terminal keys have no outgoing segment: they are skipped for easing,
and a terminal-only selection rejects that operation. Values and times remain intact.

Repeat appends copies separated by `last_selected - first_selected + 1` frames.
It preserves sparse gaps, poses and easing, extends scene duration, selects the new
copies and creates one undo step. Collisions reject all copies without overwriting;
limits are 32 copies, 100,000 new keys and frame 1,000,000. This does not synthesize a
seamless loop or separate spatial path from timing.

## Evidence and remaining scope

Six vector-domain cases, two key-pattern cases and a pressure/concave-lasso render
case extend the final suite to 65 CTest entries. Native authoring smoke exercises
constrained input, grid, lasso modifiers, clipboard shortcuts, nudge, transactional
cleanup, key patterns and semantic/pixel-equivalent save/reopen. Physical tablets,
production-size performance, raster lasso, partial contours, independent channel
keys, spatial tangents and cameras remain open. P04 and P06 remain partial.
