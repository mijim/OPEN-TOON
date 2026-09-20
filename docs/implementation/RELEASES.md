# Experimental releases

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
