# ADR-012 — Immutable media, project format 2 and raster brush boundary

Status: accepted for the experimental editor. Production P05/P11 acceptance remains open.

## Decision

Document snapshots share immutable image and raster tile buffers. A raster drawing is a finite
canvas up to 8192 × 8192 pixels with sparse 64 × 64 tiles. Each tile contains little-endian
16-bit premultiplied RGBA samples in [0, 32768], following libmypaint's 15-bit surface contract.
RGB never exceeds alpha. The current working colors are unmanaged display-referred sRGB;
this does not claim linear-light compositing or OCIO support. Imported RGBA8 images remain
separate; raster painting does not destructively edit the imported image.

libmypaint 1.6.1 is built from an exact upstream commit and SHA-256 archive pin, without
GLib, GEGL or OpenMP. The Qt-free brush adapter owns one mutable gesture, catches failures
at the C callback boundary and publishes immutable tiles. Only changed tiles are copied
for preview; one completed gesture becomes one undo command. Cancellation discards the
private surface. Ink, soft, dry, smudge and eraser settings are original programmatic presets,
not third-party brush assets. Pressure uses the engine's radius and opacity mappings.
Tilt samples cross the adapter boundary, but the built-in presets do not map tilt to a
brush property; physical tilt/eraser-end qualification remains open.

## Persistence and recovery

SQLite schema 2 stores Zstandard-compressed resources addressed by SHA-256 alongside JSON
revision metadata. OpenSSL supplies SHA-256; no custom cryptography is implemented. Resources,
revision rows and reachability references commit in one FULL-synchronous SQLite transaction.
This supersedes ADR-011's inline media and selects in-database blobs instead of ADR-004's
proposed external blob publication. It avoids a separate filesystem durability window.
The tradeoff is a larger SQLite file and synchronous compression/verification during save.

Reads validate declared length, decompression, checksum and document invariants. Existing
resources are verified before reuse. Metadata is limited to 64 MiB, individual decoded
resources to 256 MiB and total decoded media to 512 MiB. Raster gesture scratch memory is
limited to 4096 tiles (128 MiB). Document media limits count logical references conservatively.

Version 1 projects remain readable. The first version 2 save creates a distinct SQLite
backup before migration. Old revisions remain readable in the upgraded file. Compaction
requires an expected head, retains at least one revision, creates an independent backup,
and collects resources unreachable from retained revisions before VACUUM. Compaction is
explicit; opening a project does not prune its history.

The previous editor cannot read format 2 files. Use the migration backup to return to it.
No forward compatibility is claimed. Future schema versions are rejected.

## Evidence and remaining gates

Storage tests use an unchanged project from the original application, migration backups,
shared-resource revision counts, corruption, missing resources, stale writers, compaction
and process termination at transaction boundaries. Brush tests verify deterministic replay,
pressure coverage, erasing, immutable snapshots and undo/redo. Native UI smoke additionally
checks mouse painting and identical rendered pixels after save/reopen.

The CPU renderer caches at most 2048 converted tiles per render thread, retaining their
immutable source identities to prevent pointer reuse errors (approximately 96 MiB maximum).
P05 still requires production workload qualification, brush import/preset management,
raster selections/transforms, texture workflows and artist/device validation. Format 2
is an experimental compatibility boundary, not a declaration that P11 is complete.
