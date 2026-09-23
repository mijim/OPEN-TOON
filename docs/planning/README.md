# Long-term development roadmap

The immediate objective is a **Harmony Moment**: a reliable, reusable character shot
from drawing/import through cut-out, substitutions, rigging, deformers, controls,
animated dialogue, nodes and camera to preview/save/reopen/export. It is a bounded
workflow milestone before full P11/1.0, not a claim of Harmony parity.

The roadmap retains **23 phases**, **70 work packages**, **283 capabilities** and
specification ownership for **193 node/family entries**. RIG-015 adds Quick Rig to
the previous 282 requirements. No old feature was removed or declared complete.
Implementation proceeds under these contracts; bounded HM-04 composition and
HM-13 output-camera contracts are accepted. HM-03 rigid-character work remains
open for artist-led acceptance.

## Start here — one execution route

1. [NOW](NOW.md): current objective, active contract, next task and stop rule.
2. [Implementation status](../implementation/STATUS.md) and [machine-readable evidence](../implementation/status.json): what actually works and what has been accepted.
3. [FIRST-STEPS](FIRST-STEPS.md), then `python3 scripts/roadmap.py slice HM-03` or the [specific generated slice](HARMONY-MOMENT.md): recommended execution order, hard prerequisites and observable acceptance.
4. [Full phase roadmap](PHASES.md): preserved P00–P22 ownership and exit criteria. Then consult [execution rules](EXECUTION.md), [dependency register](LIBRARIES.md), [estimation assumptions](ESTIMATES.md) and [reorientation rationale](REORIENTATION.md) as needed.

The generated Harmony Moment view contains all HM-00–HM-15 contracts; its list is not itself a ready-work queue. Use `python3 scripts/roadmap.py next` and verify accepted prerequisite evidence before beginning a slice.

## What each status means

| Term | Meaning |
|---|---|
| Full roadmap P00–P22 | All 23 phase ownership areas and complete exit criteria, including long-term scope; phase numbers are not execution order. |
| Delivery plan HM-00–HM-15 | Sixteen bounded contracts for the immediate character-shot milestone, connected by `requires` dependencies. |
| Contract accepted | The bounded slice acceptance has recorded evidence, so its specific contract can satisfy a consumer prerequisite; the owning phase can still be open. |
| Working subset | Real implemented behavior with partial catalog coverage; it does not imply an accepted slice or a completed feature/phase. |
| Phase complete | Every phase exit criterion has passed with evidence. No phase currently meets this bar. |
| Historical backlog | Preserved BOOT/BASE/FILM planning IDs in [FOUNDATION-BACKLOG.md](FOUNDATION-BACKLOG.md); useful context, not the active queue. |
| Evidence and verification | Actual behavior, acceptance records and test/artistic results in [implementation status](../implementation/STATUS.md), [status.json](../implementation/status.json) and linked records; a plan, button or test that mirrors implementation is insufficient. |

`planned` describes a contract's recorded state, not readiness. A consumer cannot start until every hard prerequisite contract has accepted evidence. With one implementer, follow the recommended order in FIRST-STEPS; at most two bounded slices may be active when separate owners exist.

## Document roles

| Role | Documents | Editing rule |
|---|---|---|
| Canonical plan/catalog | `roadmap.json`, `libraries.json`, `node-assignments.json` and `docs/catalog/{features,domains,node-reference,nonfunctional}.json` | Source of IDs, scope, dependencies, assignments and estimates. |
| Generated views | `HARMONY-MOMENT.md`, `PHASES.md`, `phases/*.md`, `LIBRARIES.md` and catalog domain views | Regenerate using the existing roadmap/catalog render scripts; do not edit by hand. |
| Current execution/evidence guides | [NOW](NOW.md), [FIRST-STEPS](FIRST-STEPS.md), [STATUS](../implementation/STATUS.md) and [status.json](../implementation/status.json) | Explain the current path and record verified behavior; reconcile with canonical records. |
| Historical or reference | [FOUNDATION-BACKLOG](FOUNDATION-BACKLOG.md), [REORIENTATION](REORIENTATION.md), [ESTIMATES](ESTIMATES.md) | Retain past tasks, decision rationale and assumptions; they do not override `roadmap.py next`. |

Canonical records: [roadmap.json](roadmap.json), [libraries.json](libraries.json),
[node-assignments.json](node-assignments.json) and the [feature catalog](../catalog/features.json).
All first-party code, UI and documentation remain English. C++20 + Qt 6/QML and the
offline modular architecture remain the accepted technical direction.

## Delivery strategy

Extract **property/persistence and evaluation contracts**, then build **characters,
substitutions, bind/render foundations, deformers and portable controls**. Audio,
manual lipsync, camera and a small useful node compositor join through their actual
prerequisites. A final integrated scene/reuse/failure journey establishes the milestone.

Full early drawing/brush/paperless completion, PSD/SVG, scripting, advanced color/FX,
optimal mesh solvers and movie encoding do not block this defined profile. Its
mandatory output is PNG + PCM WAV + timing/color metadata. They remain explicit
follow-on work, never implied supported features. Existing defects that block the
shot take priority over adding conveniences.

Quick Rig generates an ordinary editable rig from artist-assigned parts/guides.
Named substitutions, masked poses, published controls and an Animator workspace are
first-class product concepts. The Rig workspace exposes full hierarchy/binding/graph
detail over the same model. No scripting prerequisite or irreversible simplified rig.

## Dependency and completion rules

- `delivery_slices[].requires` is the hard contract DAG for implementation entry.
  Planned dependencies are not delivered contracts; baseline reuse needs evidence.
- Phase `depends_on` lists whole-epic completion obligations, not a serial task queue.
  Stable phase numbers identify ownership, not mandatory chronological execution.
- Every feature still has one primary completion phase. A slice may deliver an early
  subset without satisfying the complete catalog acceptance.
- Every node entry has one specification owner. The small HM subset does not mean
  all listed families/operators are implemented or equivalent to Harmony.
- Keep at most two bounded slices active with explicit owners; one developer works
  sequentially. The dependency graph does not assume extra people exist.
- After HM, close the retained P00–P11 scope and wider qualification before claiming
  supported 1.0. P12–P22 preserve advanced rigs/automation, morphing, FX, interchange,
  3D, games, studio, optional AI/legacy and coverage maintenance.

HM-00's reproducible reference assets and foundation contract were accepted with
[owner artistic review](../implementation/HM00-REVIEW.md). The bounded
[HM-02 artwork-intake contract](../implementation/HM02-ARTWORK-INTAKE.md) is also
accepted. HM-01's bounded property contract is accepted. HM-03 now has typed persistent
characters, substitutions and coordinated view sets, but its full rigid-character
acceptance gate remains open. [HM-04](../implementation/HM04-ACCEPTANCE.md)
and [HM-13](../implementation/HM13-ACCEPTANCE.md) are accepted bounded contracts;
their owning full phases remain open. The second-animator working-shot review
remains an HM-15 gate.

## Queries and validation

```sh
python3 scripts/roadmap.py milestone HM
python3 scripts/roadmap.py slice HM-05
python3 scripts/roadmap.py next
python3 scripts/roadmap.py feature CTL-001
python3 scripts/roadmap.py phase P08
python3 scripts/roadmap.py library LIB-AUDIO
python3 scripts/roadmap.py stats
python3 scripts/catalog.py show RIG-015
python3 scripts/catalog.py render
python3 scripts/roadmap.py render
python3 scripts/validate_docs.py
python3 -m unittest discover -s tests -p 'test_roadmap.py'
```

Validators enforce feature coverage, both dependency DAGs, slice ownership and
prerequisite contracts, priority order, reciprocal feature/library assignments,
node ownership, evidence paths, terminal milestone closure and generated view freshness.
The [historical foundation backlog](FOUNDATION-BACKLOG.md) retains prior issue IDs;
it is not the current execution queue.
