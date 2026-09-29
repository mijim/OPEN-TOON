# Status audit — 2026-09-28

This audit reconciles the canonical catalog, Harmony Moment slices, phase
roadmap, implementation ledger and current local verification. Status refers
to the **full catalog acceptance** unless explicitly qualified as a bounded
delivery contract. A passing subset does not make an entire feature or phase
complete.

| Record | Current classification | Meaning |
|---|---:|---|
| Catalog capabilities | 83 `partial`, 200 `not_started`, 0 complete | DEF-001/003/010 gain bounded animated subsets; full deformer acceptance remains open. |
| Harmony Moment slices | 7 complete, 1 in progress, 8 planned | HM-00–HM-05 and HM-13 are accepted bounded contracts; HM-06 has a tested working subset. |
| Full phases P00–P22 | 10 in progress, 13 planned, 0 complete | P09 has a bounded mesh/render foundation but is not a completed deformation phase. |
| Node inventory | 193 `not_started` | Entries include families; internal HM-04 graph operators are not counted as specified, user-editable inventory effects. |
| Nonfunctional requirements | 15 `partial_evidence`, 12 `proposed_not_measured`, 0 verified | Bounded checks do not satisfy full performance, device, accessibility or release gates. |
| Open-source libraries | 9 experimentally adopted, 5 selected for plan, 16 candidates | Adopted flags match installed records; future candidates still need their consumer-specific evidence. |

The current macOS system build has 148 passing CTest entries, including native
character-control and PCM16 audio integration. Native dashboard and audio
timeline smoke checks pass. The last Windows/Linux CI evidence belongs to an
earlier source commit. The experimental.10 arm64 package was verified locally
but publication was cancelled; experimental.17 remains working source only. Physical tablet and
current cross-platform binary qualification remain open.

This audit corrected camera delivery mistakenly listed under raster phase P05
in `status.json`, moved it to animation/camera phase P06, refreshed P08 rigid
character and P10 composition descriptions, removed obsolete HM-04 blockers,
and recorded current camera/graph verification. It also changed catalog entries
that had working subsets but still said `not_started`: typed visual layers,
exposure-preserving drawing ownership, pegs, rigid character breakdown,
substitutions, coordinated views, internal alpha mattes, camera and graph
subsets. Studio color configuration, editable nodes/cutters, full character
poses, audio, advanced deformers and multiplane remain unimplemented where their
specific behavior has not been delivered.

The owner delegated the HM-03 19-part visual workflow review to the implementer.
The original PNG assembly matches reference pixels after paint-order setup;
coordinated views, substitutions, reparenting, undo and reopened output have
[bounded acceptance evidence](HM03-ACCEPTANCE.md). P08 and the independent
second-animator HM-15 journey remain open.

[HM-05](HM05-ACCEPTANCE.md) adds a static mesh bind/render subset with
per-substitution format-8 identity, source-vector retention, a bounded image
proxy, native mouse editing, checker and original 19-part render evidence.
DEF-008 and DEF-015 are partial. [HM-06 progress](HM06-PROGRESS.md) adds
format-9 keyed bone/curve subsets, native handles and independent substitution
chains; DEF-001, DEF-003 and DEF-010 are partial. Influence regions remain
not started. Extreme-bend quality and the control-to-preview budget still
block HM-06 acceptance.

`scripts/validate_docs.py` now checks evidence files for nonempty partial
statuses, library adoption flags, P00–P11 phase-status agreement, the source
version, CTest totals and generated views. The complete feature-specific
limits remain in [`features.json`](../catalog/features.json); the current
implementation evidence is in [`status.json`](status.json), with phase detail
in [`STATUS.md`](STATUS.md). Regenerate catalog and roadmap views after editing
their canonical JSON sources.
