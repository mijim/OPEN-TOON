# ADR-022 — Character-first delivery contracts

Status: accepted planning direction, 2026-09-21. Implementation pending. No format,
runtime, library lock or completed capability is changed by this ADR.

## Context

The current editor has useful drawing, full-pose animation and persistence subsets.
Broad phase dependencies serialize unrelated drawing, camera, media, deformation,
compositor and scripting work. The owner prioritizes a reliable character-animation
workflow while retaining the long-term catalog.

## Decision

Keep phase/feature IDs stable and introduce a validated capability-slice DAG in
`docs/planning/roadmap.json`. Every hard edge names a delivered contract and every
slice has an owner, scope, acceptance, exclusion, source evidence and labor range.
Only accepted prerequisite evidence permits dependent implementation. Phase completion
and the bounded Harmony Moment are separate claims. No broad earlier phase is a
surrogate prerequisite for a subset that does not use it.

Extract these foundations before their consumers:

1. Stable character/part/property identity; typed numeric and held discrete channels;
   rest/setup/authored/evaluated separation; migration and undo/recovery invariants.
2. A shared evaluation boundary with explicit driver/hierarchy/solver/deformation/
   attachment ordering and typed image/transform/matte connections. Reject cycles and
   multiple-driver ambiguity; keep the image DAG distinct from the transform hierarchy.
3. Rest mesh, UV and per-substitution bind intent plus an actually functioning texture
   deformation renderer. Original artwork remains editable; render proxies declare
   their resolution and unsupported geometry.
4. Character-scoped substitution sets and masked poses, then declarative controls and
   guided Quick Rig. Simple drivers do not require a scripting runtime. In the initial
   profile, widgets key their source control properties; driven properties expose
   ownership and reject conflicting direct writes unless explicitly unbound.
5. Rational audio-device/live and offline timing, explicit output camera and a limited
   linear-sRGB/premultiplied pipeline. Legacy scenes retain an explicit compatible
   appearance profile; conversion is deliberate and tested, not implicit recoloring.
6. Dependency-closed reusable rigs: stable role mappings and complete ID remapping
   across drawings, variants, palettes, poses, deformers, controls, graph and media.

Each persisted addition requires a concrete schema/migration design in its owning
slice before implementation. Preserve format-3 evaluated animation and backup old
files; do not speculatively serialize empty future systems. Keep document/application
logic Qt-free and library adapters narrow.

## Consequences

Quick Rig is a new catalog requirement (RIG-015). Declarative controls/guides move
from P13 to P08; core IK moves to P09; lip-sync ownership moves to P07; scripted
controllers stay behind P12. Node evaluation foundations can precede deformation,
while audio, camera and mask/editor work branch after their own contracts.

Full paperless/vector/raster breadth, optimal topology, PSD/SVG import, envelope and
advanced solvers, 2D pose grids, movie codecs, managed HDR, scripting and studio
extensions remain required in their long-term phases. Their incompleteness does not
block a declared PNG/WAV, orthographic, bone/curve character profile. Safety, save/
reopen, alpha, clock correctness and a viable deformation render path are not deferred.

No measured performance advantage or Harmony compatibility is claimed. A second
animator must finish both original-art and imported-part routes, and reuse the rig in
a second scene, before the milestone is accepted. Full P11/1.0 retains broader phase,
platform, installation and device gates.

## Alternatives rejected

- Exhaust every early phase: delays the character workflow on unrelated tools.
- Jump directly to a rig UI: masks missing persistence, binding and rendering contracts.
- Require scripts for every control: creates avoidable runtime/API dependencies.
- Fork an entire animation application or swap renderers without evidence: retains
  unbounded integration risk; reuse candidates still require scoped measurements.
- Declare completion from a happy-path demo: does not establish a reusable reliable rig.

See the [rationale and baseline audit](../../planning/REORIENTATION.md) and
[generated delivery contracts](../../planning/HARMONY-MOMENT.md).
