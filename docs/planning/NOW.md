# Current work — Harmony Moment

**Current execution guide; status snapshot as of 2026-09-23.** This page is a route into the [canonical delivery contracts](roadmap.json) and [recorded implementation evidence](../implementation/status.json), not another source of truth. Re-run `python3 scripts/roadmap.py next` before taking work; if this snapshot differs, use the canonical records and reconcile this page.

Read in order: **this page → [implementation status](../implementation/STATUS.md) → [next-task guide](FIRST-STEPS.md) → [the specific slice](HARMONY-MOMENT.md) → [full roadmap and references](README.md).**

The immediate objective is a complete, reliable character-animation **Harmony Moment**, from artwork intake through rigging, deformation, controls, animation and audio, composition and camera to preview, save/reopen and export. This is a bounded workflow milestone; the full P00–P22 roadmap remains intact.

**Active critical-path slice: [HM-03](HARMONY-MOMENT.md#hm-03).** HM-01 typed property persistence and HM-02 artwork intake are accepted prerequisites. The rigid character, substitutions and coordinated views have a working subset. The next recorded task is the HM-03-B artist review of 19-part assembly and view switching, followed by qualification of animated-ancestor reparenting or an explicit bounded behavior and resolution of observed workflow blockers. Do not accept HM-03 on implementation or tests alone.

**Current gate:** HM-03 has no completion evidence yet; artist-led rigid-character acceptance remains pending. HM-05, HM-07, HM-11 and HM-12 require its accepted contract, with further slices downstream. HM-10 audio is independently eligible through accepted HM-01 and may occupy a second slot with a separate owner. With one implementer, follow the [recommended priority](FIRST-STEPS.md) and finish HM-03 first.

**Evidence required for HM-03:** demonstrate the [slice's observable acceptance](HARMONY-MOMENT.md#hm-03) on the 19-part fixture, including identity-safe character/view changes, supported preserve-world reparenting or explicit rejection of unsupported transforms, atomic variant switching and missing-member reporting, undo, save/reopen and relevant failure cases. Record artist review and verification in the implementation evidence before treating the contract as accepted. The owning P08 phase remains open afterward until all its exit criteria pass.

## Recorded dependency snapshot

Statuses and hard prerequisites below come from `roadmap.json`; acceptance and phase limits come from [implementation status](../implementation/STATUS.md). `planned` is not `ready`.

| Slice | Recorded state | Hard prerequisite contracts | Operational gate |
|---|---|---|---|
| [HM-03](HARMONY-MOMENT.md#hm-03) | In progress; working subset | HM-01, HM-02 accepted | Artist acceptance pending. |
| [HM-04](HARMONY-MOMENT.md#hm-04) | Bounded contract accepted | HM-01 accepted | P10 owning phase remains open. |
| [HM-05](HARMONY-MOMENT.md#hm-05) | Planned | HM-03, HM-04 | Wait for HM-03 acceptance. |
| [HM-06](HARMONY-MOMENT.md#hm-06) | Planned | HM-05 | Wait for HM-05 acceptance. |
| [HM-07](HARMONY-MOMENT.md#hm-07) | Planned | HM-03, HM-04 | Wait for HM-03 acceptance. |
| [HM-08](HARMONY-MOMENT.md#hm-08) | Planned | HM-03, HM-07 | Wait for both contracts. |
| [HM-09](HARMONY-MOMENT.md#hm-09) | Planned | HM-06, HM-07, HM-08 | Wait for all three contracts. |
| [HM-10](HARMONY-MOMENT.md#hm-10) | Planned; independently eligible | HM-01 accepted | May start with a separate owner. |
| [HM-11](HARMONY-MOMENT.md#hm-11) | Planned | HM-03, HM-10 | Wait for both contracts. |
| [HM-12](HARMONY-MOMENT.md#hm-12) | Planned | HM-04, HM-03 | Wait for HM-03 acceptance. |
| [HM-13](HARMONY-MOMENT.md#hm-13) | Bounded contract accepted | HM-04 accepted | P06 owning phase remains open. |
| [HM-14](HARMONY-MOMENT.md#hm-14) | Planned | HM-10, HM-12, HM-13 | Wait for HM-10 and HM-12 acceptance. |
| [HM-15](HARMONY-MOMENT.md#hm-15) | Planned | HM-09, HM-11, HM-14 | Wait for the three joining contracts. |

**Stop rule:** before implementing a consumer slice, inspect `requires` and accepted evidence for every prerequisite. If any required contract is unaccepted, stop consumer work and finish that prerequisite or choose a truly eligible independent slice. An isolated feasibility spike may inform a decision, but it does not make its consumer ready. Keep at most two bounded slices active with separate owners.

## Consult and validate

```sh
python3 scripts/roadmap.py next             # Current eligible slices and instruction
python3 scripts/roadmap.py slice HM-03      # One contract, prerequisites and acceptance
python3 scripts/roadmap.py phase P08        # Owning phase and exit criteria
python3 scripts/roadmap.py feature RIG-006  # Primary completion assignment
python3 scripts/catalog.py show RIG-006    # Feature behavior and acceptance
python3 scripts/roadmap.py stats            # Plan counts and effort envelope
python3 scripts/validate_docs.py           # Documentary and generated-view consistency
```

For observed behavior and verification, consult [STATUS.md](../implementation/STATUS.md), [status.json](../implementation/status.json) and the linked acceptance records. The historical backlog and planning rationale are reference material, not a competing task queue.
