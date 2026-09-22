# ADR-027 — Typed composition kernel and format-6 profile

Status: experimental implementation for HM-04, 2026-09-23. HM-04 remains open.

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

## Boundaries and next evidence

This is an 8-bit CPU reference path, not a full color-management system. At
fractional resizes, isolating layers before `Over` can differ from the direct
legacy painter. The profile is opt-in; old scenes remain exact on their old
path. Alpha fixtures, graph rejection and save/reopen tests cover the bounded
behavior. HM-04 still needs revision-aware cache/job publication, measured
production workloads, and a stated solve ordering across later deformer,
attachment and camera consumers before its contract can be accepted. No full
node editor, effects catalog, HDR/EXR or OCIO manager is claimed.
