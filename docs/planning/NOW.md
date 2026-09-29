# Current work — Harmony Moment

**Current execution guide; status snapshot as of 2026-09-29.** This page is a route into the [canonical delivery contracts](roadmap.json) and [recorded implementation evidence](../implementation/status.json), not another source of truth. Re-run `python3 scripts/roadmap.py next` before taking work; if this snapshot differs, use the canonical records and reconcile this page.

Read in order: **this page → [implementation status](../implementation/STATUS.md) → [next-task guide](FIRST-STEPS.md) → [the specific slice](HARMONY-MOMENT.md) → [full roadmap and references](README.md).**

The immediate objective is a complete, reliable character-animation **Harmony Moment**, from artwork intake through rigging, deformation, controls, animation and audio, composition and camera to preview, save/reopen and export. This is a bounded workflow milestone; the full P00–P22 roadmap remains intact.

**Active critical-path slice: [HM-06](HARMONY-MOMENT.md#hm-06).** HM-05 has accepted bounded [rest/UV and renderer evidence](../implementation/HM05-ACCEPTANCE.md). The [working HM-06 subset](../implementation/HM06-PROGRESS.md) now saves bone/curve controls, weights and keys, supports canvas drags, continuous limbs and explicit matching of compatible substitution poses. The 480-frame visual shot passes continuity, native input and reopen checks. Next, review further extreme bends and the complete reference shot before accepting the slice.

**Independent active slice: [HM-07](HARMONY-MOMENT.md#hm-07).** Format-12 named masked poses capture, apply and blend selected channels and substitutions with undo/reopen evidence. [The tested subset](../implementation/HM07-PROGRESS.md) leaves controls, cross-rig transfer and workspace views open.

**Current gate:** HM-06 has animated bone/curve evaluation, bounded per-substitution binding and editable elbow influence; broader extreme-bend quality and artistic interaction remain unaccepted. HM-07 controls, HM-10 audio and HM-12 nodes are independently eligible; with one implementer, follow the [recommended priority](FIRST-STEPS.md) and finish HM-06 first.

**Accepted HM-03 evidence:** the owner delegated review to the implementer. The original 19-part import/assembly/view journey, reference-pixel comparison, saved/reopened substitutions, independent copy, undo, negative/nonuniform reparenting and visual screenshot inspection are recorded in [HM03-ACCEPTANCE](../implementation/HM03-ACCEPTANCE.md). This bounded contract does not complete P08 or the independent second-animator HM-15 review.

## Recorded dependency snapshot

Statuses and hard prerequisites below come from `roadmap.json`; acceptance and phase limits come from [implementation status](../implementation/STATUS.md). `planned` is not `ready`.

| Slice | Recorded state | Hard prerequisite contracts | Operational gate |
|---|---|---|---|
| [HM-03](HARMONY-MOMENT.md#hm-03) | Bounded contract accepted | HM-01, HM-02 accepted | P08 owning phase remains open. |
| [HM-04](HARMONY-MOMENT.md#hm-04) | Bounded contract accepted | HM-01 accepted | P10 owning phase remains open. |
| [HM-05](HARMONY-MOMENT.md#hm-05) | Bounded contract accepted | HM-03, HM-04 accepted | P09 owning phase remains open. |
| [HM-06](HARMONY-MOMENT.md#hm-06) | In progress | HM-05 accepted | Extreme-bend quality and interaction budgets remain. |
| [HM-07](HARMONY-MOMENT.md#hm-07) | In progress | HM-03, HM-04 accepted | Named masked poses and blend slider pass; controls and workspaces remain. |
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
