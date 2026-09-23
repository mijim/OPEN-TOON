# ADR-027 — Typed composition kernel and format-6 profile

Status: accepted bounded HM-04 implementation, 2026-09-23. P10 remains open.

## Decision

Keep old scenes on an explicit `LegacyQt` composition profile. Format 6 stores the
profile in the document and defaults earlier formats to `LegacyQt`; the next
successful save of a format-5 project makes a `.pre-v5.bak` backup. Switching the
profile is one undoable document command. The View menu exposes the choice.

Compile the current ordered drawing layers into a small, typed image graph with
separate Display and Write terminals. Graph validation checks IDs, input slots,
source existence, port types, output kinds and cycles before evaluation. Only
ancestors of the requested output are evaluated. Layer
edits can identify downstream nodes through `affectedByLayer`. A separate
transform port reads evaluated local pose without treating transform data as an
image. This graph is derived for now; no editable node topology is serialized.

The new `LinearSrgb` path renders each visible drawing layer to premultiplied
ARGB, decodes channels to linear sRGB, composites `Over`, then encodes to
premultiplied sRGB. Zero-alpha output is black-transparent. A matte is extracted
from image alpha and can multiply another image's premultiplied channels and
alpha. Display and Write use the same graph evaluator; legacy scenes continue
through the existing direct painter to preserve their established appearance.
Qt remains behind the render adapter; graph types and validation are Qt-free.

## Evaluation order and revision boundary

The retained solve order is: authored keys and future property drivers → local
transforms → parent hierarchy → future deformation → future scene-space
attachments → output camera → layer rasterization → typed image graph
→ Display or Write. A driver must resolve before its consumer; the future
deformer samples the completed scene-space transform and rest binding; the
camera maps scene to output space once; image nodes cannot feed a Transform
port. Cycles within or between these stages must fail validation rather than
depend on iteration order. Authored keys, hierarchy, orthographic camera,
rasterization and the image graph are executable. This order defines where
the later HM-05 and HM-09 contracts connect; ADR-028 defines the camera mapping.

Canvas composition now uses a 96 MiB least-recently-used cache keyed by scene
generation, document revision, frame, size, profile, output and view options.
Transient brush/pose previews bypass it. A new revision drops old images; a
render ticket may publish only while its request and revision remain current.
The canvas renders a missed current frame synchronously and speculatively queues
the next frame on one background worker. Its immutable document snapshot, latest-only
pending slot, cancellation and render tickets keep superseded work out of the cache.
Oversized frames do not enter the speculative queue. Export uses its existing
immutable snapshot; a cancellation callback can stop a linear frame
between graph nodes or pixel rows and records a partial cancelled export.
Topological ordering and invalidation use iterative traversal, including
images below a changed parent peg. Global state changes remain conservatively
covered by revision-wide canvas invalidation.

The linear path uses 8-bit sRGB lookup tables and conservative transformed ink
bounds for source-over work. This changes only the opt-in profile. Rotated
vector and sparse raster alpha coverage is compared against the direct painter;
the [measured synthetic workload](../../implementation/COMPOSITOR-BENCHMARK.md)
records synthetic and original-art throughput and the preview budget. The original
19-part sRGB fixture retains alpha coverage at half resolution and matches
Display/Write/reopened pixels at three poses.

## Boundaries and next evidence

This is an 8-bit CPU reference path, not a full color-management system. At
fractional resizes, isolating layers before `Over` can differ from the direct
legacy painter. The profile is opt-in; old scenes remain exact on their old
path. Alpha fixtures, graph rejection and save/reopen tests cover the bounded
behavior. The bounded HM-04 contract has native playback/scrub, animated
original-art cost and alpha/color chart evidence in HM04-ACCEPTANCE.md. No full
node editor, effects catalog, HDR/EXR or OCIO manager is claimed.
