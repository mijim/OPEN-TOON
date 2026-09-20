# Experimental releases

## 0.2.0-experimental.10 — vector authoring and animation patterns

- Whole-vector lasso with additive/subtractive selection, select all and invert.
- Independent vector cut/copy/paste between drawings/scenes with preserved colors.
- Alignment, center distribution, stable art-layer stacking and keyboard nudges.
- Batch width, palette, art-layer and fill edits in selection-owned Properties.
- Adjustable corner-aware smoothing and pressure-aware sampled-pencil simplification.
- Line tool, constrained primitives and drawing-local grid/snapping.
- Batch full-pose interpolation/easing and collision-safe key-block repetition.
- Professional project README and expanded English workflows/contracts.
- Verified local macOS arm64 preview, macOS 15 minimum, bundled runtime and notices,
  matching dependency sources/recipes, checksums and verified ad-hoc signatures.

Binary publication was cancelled at the owner's request; this tag distributes source only.
The locally prepared package remains available in the development workspace.

65/65 CTest entries and all native authoring/animation smoke journeys passed. The
release ZIP also passes those native journeys after extraction outside the workspace,
with no external Homebrew or developer-home runtime libraries loaded. Format 3 remains
unchanged; no new application library was adopted. P04/P06 remain partial. Physical
tablets, Intel, clean-machine validation and new Windows/Linux binaries remain pending.
The macOS preview is not Developer ID signed or Apple-notarized. See [ADR-021](../architecture/adr/021-vector-authoring-and-key-patterns.md)
and [installation notes](MACOS-PREVIEW.md).

## 0.2.0-experimental.9 — additive vector selection

- Shift adds and Alt subtracts whole vectors by click or rectangular marquee.
- Sparse groups retain their members when switching Select/Marquee; dotted bounds and Properties show the actual selection.
- Direct group move/scale/rotation keeps enclosed bystanders untouched.
- Duplicate selects only new copies, including overlapping copies; previews cancel and edits undo atomically.
- macOS: 56 CTest entries and native selection/animation workflows passed. Source only; format 3 unchanged; no new dependencies.

VEC-009/P04 remains partial: no lasso, partial contour selection or raster masks. See ADR-020.

## 0.2.0-experimental.8 — canvas motion positions

- Animate → Path: click/drag pose markers on the canvas; double-click the trajectory to add a sampled pose.
- Fixed local drawing reference; parent-aware movement through rotated, negative and nonuniform transforms.
- Screen-space picking, frame label, Shift direction constraint and transactional preview/undo.
- Changes only X/Y; retains timing, other transform channels, easing, artwork and parent animation.
- View changes cancel active path drags; singular parents and locked layers reject edits.
- macOS: 55 CTest entries; native trajectory, hierarchy, reference-stability and persistence checks passed. Source only; format 3 unchanged.

This is a partial ANI-008/P06 implementation. Spatial spline tangents and separate
velocity remain pending. Adding a pose can change interpolation between keys. See ADR-019.

## 0.2.0-experimental.7 — direct pose-key blocks

- Shared selection across Curves, Timeline and Xsheet, with span/toggle/box gestures.
- Move selected diamonds together, Alt-drag to duplicate, drag the right edge to stretch timing.
- Collision-safe local-pose copy/paste between layers preserves pivots/easing without drawings.
- Focus-aware copy/paste, select all, key nudges and Delete; Escape or selection changes cancel previews.
- One undo per committed operation, existing format 3 and no new dependencies.
- macOS: 54 CTest entries plus native group editing, shortcuts and persistence checks. Source only.

P06 remains partial: no independent channel times, cross-layer batch selection,
world-space retargeting or cameras. See ADR-018 for the exact supported scope.

## 0.2.0-experimental.6 — editable combined motion

- All motion is the default: highlight a channel and edit square keys or round handles in place.
- Linked easing shapes all channels together; switch it off for individual handle edits.
- Full-pose diamonds drag in All motion, Timeline and Xsheet; Keys mode supports double-click insertion.
- Time zoom/panning, wider timeline cells, larger hit areas and explicit add/remove controls.
- Clear removes selected exposures and animation keys together, with one undo.
- Double-click pencil/polygon segments to insert points; Delete/Backspace removes a selected point.
- macOS: 50 CTest entries; native combined-curve, key drag/add, point and Clear workflows passed. Source only; format remains 3.

P04/P06 and P11 remain partial. Independent channel timing, analytic contour geometry,
spatial velocity, cameras and drawing morphing remain open. See ADR-017.

## 0.2.0-experimental.5 — visual animation and precise picking

- Animate (A): move, scale and rotate layer poses on the canvas, with initial anchors, live previews and one undo per gesture.
- Integrated Bezier handles, channel easing/overshoot presets, double-click graph keys and a visible canvas trajectory.
- Constant eight-screen-pixel vector picking margin; distinct tool cursors and contextual move/resize/rotate/point feedback.
- Live point-edit previews, usable thin-line handles and a scrollable tool strip.
- Compact shared controls, a single curve toolbar, optional Values fields and a working draggable panel divider.
- Format 3 curve persistence, original backups for schema 1/2 upgrades and current-format compaction.
- macOS: 48 CTest entries plus native mouse pose, point and Bezier workflows. Source only.

Drawing morphing, contour Bezier geometry, independent channel key times, separate spatial velocity and cameras remain open. Older editors cannot open format 3; use the migration backup with them. See ADR-016.

## 0.2.0-experimental.4 — direct editing workspace

- Removed separate selection, timing and curve dialogs.
- Canvas box handles move, scale and rotate with live preview; Shift constrains
  proportions/snaps angle, Escape cancels, release creates one undo command.
- Properties follows selected objects/regions or explicitly selected layers.
  Numeric fields rebind after edits; single vectors expose width, fill, art and color.
- Curves and timing controls remain in the resizable bottom workspace. Timeline and
  Xsheet range-end handles stretch timing directly.
- Retina transform overlay alignment corrected during visual verification.
- macOS: 43 CTest entries plus native handle/Properties/timing journeys. Source only.

Raster free transforms currently use nearest-neighbor sampling. Rotated ellipses
become 128-point editable polygons. Lasso, soft masks and production-scale transform
performance remain pending; see ADR-015. No phase completion or binary installer claim.

## 0.2.0-experimental.3 — P04/P05 source release

- Rectangular selection of fully enclosed vector strokes and/or raster pixels.
- Integer move/duplicate/delete, horizontal/vertical flips and lossless 90° rotation.
- Shared immutable tiles, alpha-safe overlapping moves, guarded canvas bounds and undo.
- Raster brush opacity; eraser strength independent of palette alpha.
- English selection controls, shortcut M and explicit mixed-media behavior documentation.
- macOS end-of-block validation: 41 CTest entries and native selection move/cancel,
  undo/redo and pixel-identical save/reopen. No Windows/Linux builds or installers.

P04/P05 remain partial: no topology fill, lasso, soft masks, arbitrary selection
rotation/scaling, brush texture management or ABR importer. Imported image assets are
excluded from this selection tool; vector geometry remains editable.

## 0.2.0-experimental.2 — source release

- Explicit Setup/Animate modes and guarded Auto key.
- Keyed/interpolated/held inspector state and previous/next key navigation.
- Pose-channel curve graph with numeric edits, mouse dragging and single-command undo.
- Key-only multi-layer retiming with collision rejection and unchanged exposures.
- English usage and animation contracts; no format or dependency changes.
- Verification at block end on macOS: 36 CTest entries, native drawing/range checks
  and curve drag/undo. Fixed a QML final-property naming conflict during validation.
- Native multi-OS CI is manual; no Windows/Linux build or installer for this source release.

P11 remains incomplete. Keys still store whole poses; independent channel timing,
editable tangents, cameras, audio, advanced rigs, deformation and nodes remain open.

## 0.2.0-experimental.1

An incremental development release on the path to P11. It does not complete P11 or
establish production equivalence with Harmony Premium. The full phase obligations
remain in [STATUS.md](STATUS.md) and the canonical roadmap.

### Available changes

- Multi-layer timeline/Xsheet ranges; linked or independent drawing paste, keys-only
  paste, cycles, timing on ones/twos/threes, guarded stretch and Alt-drag overwrite.
- Scene markers and paginated Xsheet PDF export (200-page cap).
- MyPaint raster ink, soft, dry, smudge and eraser presets, with mouse or pressure input,
  per-gesture undo, cancellation and the same pixels in preview, save/reopen and PNG export.
- Sparse immutable raster tiles; unchanged media shared by undo and saved revisions.
- Compressed SHA-256-checked resources in project format 2, version 1 migration backups,
  explicit project compaction with full backup and background recovery snapshots.
- Pinned dependency additions and source/license records, native range/brush workflow
  checks, migration/corruption/compaction tests and a reproducible synthetic 4K benchmark.

### Verification

[Source commit c07414f passed CI](https://github.com/mijim/OPEN-TOON/actions/runs/35531337766)
on Windows Server 2022, macOS 14 and Ubuntu 24.04 with Qt 6.8.3. The suite has
33 CTest entries on macOS/Linux and 32 on Windows (the POSIX process-termination
case is excluded). Linux additionally passes 27 sanitizer cases. macOS runs native
mouse/pressure, raster pixel round-trip, timeline selection/Alt-drag and undo checks.
The subsequent release commit updates documentation and evidence only.

### Compatibility and limits

The previous editor cannot open format 2. A version 1 project's first new save leaves
an adjacent `.pre-v2.bak` that remains readable by the previous version. Keep that
backup until the upgraded project is accepted. Compaction also preserves the original
history in an adjacent backup.

This release remains a source/build-tree application. Public standalone installers,
signing/notarization, physical tablets and clean-machine deployment are unqualified.
The advanced vector, camera, audio/video, rig library, deformation and node/color
pipeline requirements are still open. Raster import transforms, ABR/texture support
and configurable brush dynamics are not yet available. Consult the guide for exact
available controls.

The screenshot `raster-editor.png` is generated by the native input workflow using procedural test strokes. It contains no imported artwork or private project content.
