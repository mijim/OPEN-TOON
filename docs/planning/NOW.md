# Current work — Harmony Moment

**Current execution guide; status snapshot as of 2026-09-28.** This page is a route into the [canonical delivery contracts](roadmap.json) and [recorded implementation evidence](../implementation/status.json), not another source of truth. Re-run `python3 scripts/roadmap.py next` before taking work; if this snapshot differs, use the canonical records and reconcile this page.

Read in order: **this page → [implementation status](../implementation/STATUS.md) → [next-task guide](FIRST-STEPS.md) → [the specific slice](HARMONY-MOMENT.md) → [full roadmap and references](README.md).**

The immediate objective is a complete, reliable character-animation **Harmony Moment**, from artwork intake through rigging, deformation, controls, animation and audio, composition and camera to preview, save/reopen and export. This is a bounded workflow milestone; the full P00–P22 roadmap remains intact.

**Active critical-path slice: [HM-05](HARMONY-MOMENT.md#hm-05).** Its HM-03 rigid-character and HM-04 graph prerequisites have accepted bounded evidence. The next task is to prove saved rest mesh/UV/binding data and actual texture-warp rendering against the checker and character fixtures, with measured cost and explicit rejection of degenerate or incompatible data. Consult the library register before choosing a dependency.

**Current gate:** HM-05 has no deformation implementation or completion evidence. HM-06 depends on its accepted representation and measured renderer path. HM-07 controls, HM-10 audio and HM-12 nodes are also eligible after HM-03; with one implementer, follow the [recommended priority](FIRST-STEPS.md) and work HM-05 first.

**Accepted HM-03 evidence:** the owner delegated review to the implementer. The original 19-part import/assembly/view journey, reference-pixel comparison, saved/reopened substitutions, independent copy, undo, negative/nonuniform reparenting and visual screenshot inspection are recorded in [HM03-ACCEPTANCE](../implementation/HM03-ACCEPTANCE.md). This bounded contract does not complete P08 or the independent second-animator HM-15 review.

## Recorded dependency snapshot

Statuses and hard prerequisites below come from `roadmap.json`; acceptance and phase limits come from [implementation status](../implementation/STATUS.md). `planned` is not `ready`.

| Slice | Recorded state | Hard prerequisite contracts | Operational gate |
|---|---|---|---|
| [HM-03](HARMONY-MOMENT.md#hm-03) | Bounded contract accepted | HM-01, HM-02 accepted | P08 owning phase remains open. |
| [HM-04](HARMONY-MOMENT.md#hm-04) | Bounded contract accepted | HM-01 accepted | P10 owning phase remains open. |
| [HM-05](HARMONY-MOMENT.md#hm-05) | Planned; eligible | HM-03, HM-04 accepted | Next critical-path slice. |
| [HM-06](HARMONY-MOMENT.md#hm-06) | Planned | HM-05 | Wait for HM-05 acceptance. |
| [HM-07](HARMONY-MOMENT.md#hm-07) | Planned; eligible | HM-03, HM-04 accepted | Controls may follow HM-05 with one implementer. |
| [HM-08](HARMONY-MOMENT.md#hm-08) | Planned | HM-03, HM-07 | Wait for both contracts. |
| [HM-09](HARMONY-MOMENT.md#hm-09) | Planned | HM-06, HM-07, HM-08 | Wait for all three contracts. |
| [HM-10](HARMONY-MOMENT.md#hm-10) | Planned; independently eligible | HM-01 accepted | May start with a separate owner. |
| [HM-11](HARMONY-MOMENT.md#hm-11) | Planned | HM-03 accepted, HM-10 pending | Wait for HM-10. |
| [HM-12](HARMONY-MOMENT.md#hm-12) | Planned; eligible | HM-04, HM-03 accepted | Node editing may follow HM-05 with one implementer. |
| [HM-13](HARMONY-MOMENT.md#hm-13) | Bounded contract accepted | HM-04 accepted | P06 owning phase remains open. |
| [HM-14](HARMONY-MOMENT.md#hm-14) | Planned | HM-10, HM-12, HM-13 | Wait for HM-10 and HM-12 acceptance. |
| [HM-15](HARMONY-MOMENT.md#hm-15) | Planned | HM-09, HM-11, HM-14 | Wait for the three joining contracts. |

**Stop rule:** before implementing a consumer slice, inspect `requires` and accepted evidence for every prerequisite. If any required contract is unaccepted, stop consumer work and finish that prerequisite or choose a truly eligible independent slice. An isolated feasibility spike may inform a decision, but it does not make its consumer ready. Keep at most two bounded slices active with separate owners.

## Consult and validate

```sh
python3 scripts/roadmap.py next             # Current eligible slices and instruction
python3 scripts/roadmap.py slice HM-05      # One contract, prerequisites and acceptance
python3 scripts/roadmap.py phase P09        # Owning phase and exit criteria
python3 scripts/roadmap.py feature DEF-008  # Primary completion assignment
python3 scripts/catalog.py show DEF-008    # Feature behavior and acceptance
python3 scripts/roadmap.py stats            # Plan counts and effort envelope
python3 scripts/validate_docs.py           # Documentary and generated-view consistency
```

For observed behavior and verification, consult [STATUS.md](../implementation/STATUS.md), [status.json](../implementation/status.json) and the linked acceptance records. The historical backlog and planning rationale are reference material, not a competing task queue.
