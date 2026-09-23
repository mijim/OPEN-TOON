# Status audit — 2026-09-23

This audit reconciles the canonical catalog, Harmony Moment slices, phase
roadmap, implementation ledger and current local verification. Status refers
to the **full catalog acceptance** unless explicitly qualified as a bounded
delivery contract. A passing subset does not make an entire feature or phase
complete.

| Record | Current classification | Meaning |
|---|---:|---|
| Catalog capabilities | 78 `partial`, 205 `not_started`, 0 complete | Working subsets have an evidence path and a stated remaining scope. |
| Harmony Moment slices | 5 complete, 1 in progress, 10 planned | HM-00, HM-01, HM-02, HM-04 and HM-13 are accepted bounded contracts; HM-03 remains open. |
| Full phases P00–P22 | 9 in progress, 14 planned, 0 complete | The implementation ledger covers P00–P11; later phases remain in the roadmap. |
| Node inventory | 193 `not_started` | Entries include families; internal HM-04 graph operators are not counted as specified, user-editable inventory effects. |
| Nonfunctional requirements | 15 `partial_evidence`, 12 `proposed_not_measured`, 0 verified | Bounded checks do not satisfy full performance, device, accessibility or release gates. |
| Open-source libraries | 9 experimentally adopted, 5 selected for plan, 16 candidates | Adopted flags match installed records; future candidates still need their consumer-specific evidence. |

The current macOS locked build has 99 passing CTest entries, a passing native
mouse/composition/camera workflow and 20 export integration cases within one
CTest entry. The last Windows/Linux CI evidence belongs to an earlier source
commit. The experimental.10 arm64 package was verified locally but publication
was cancelled; experimental.15 remains source only. Physical tablet and
current cross-platform binary qualification remain open.

This audit corrected camera delivery mistakenly listed under raster phase P05
in `status.json`, moved it to animation/camera phase P06, refreshed P08 rigid
character and P10 composition descriptions, removed obsolete HM-04 blockers,
and recorded current camera/graph verification. It also changed catalog entries
that had working subsets but still said `not_started`: typed visual layers,
exposure-preserving drawing ownership, pegs, rigid character breakdown,
substitutions, coordinated views, internal alpha mattes, camera and graph
subsets. Studio color configuration, editable nodes/cutters, full character
poses, audio, deformers and multiplane remain unimplemented where their
specific behavior has not been delivered.

`scripts/validate_docs.py` now checks evidence files for nonempty partial
statuses, library adoption flags, P00–P11 phase-status agreement, the source
version, CTest totals and generated views. The complete feature-specific
limits remain in [`features.json`](../catalog/features.json); the current
implementation evidence is in [`status.json`](status.json), with phase detail
in [`STATUS.md`](STATUS.md). Regenerate catalog and roadmap views after editing
their canonical JSON sources.
