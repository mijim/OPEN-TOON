# Experimental releases

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
