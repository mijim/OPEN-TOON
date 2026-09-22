# Long-term development roadmap

The immediate objective is a **Harmony Moment**: a reliable, reusable character shot
from drawing/import through cut-out, substitutions, rigging, deformers, controls,
animated dialogue, nodes and camera to preview/save/reopen/export. It is a bounded
workflow milestone before full P11/1.0, not a claim of Harmony parity.

The roadmap retains **23 phases**, **70 work packages**, **283 capabilities** and
specification ownership for **193 node/family entries**. RIG-015 adds Quick Rig to
the previous 282 requirements. No old feature was removed or declared complete.
Implementation proceeds under these contracts; bounded HM-03 rigid-character work is underway.

## Start here

1. [Harmony Moment delivery plan](HARMONY-MOMENT.md): generated scope, hard contract
   dependencies, acceptance profile, 16 slices and remaining effort.
2. [Reorientation rationale](REORIENTATION.md): current code audit, dependency changes,
   tradeoffs, intended differentiation and primary references.
3. [Next tasks](FIRST-STEPS.md): executable order and permitted parallel work.
4. [Full phase roadmap](PHASES.md): preserved long-term ownership and exit criteria.
5. [Dependency register](LIBRARIES.md), [execution rules](EXECUTION.md) and
   [estimation assumptions](ESTIMATES.md).
6. [Implementation evidence](../implementation/status.json): what actually works.

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
acceptance gate remains open. The second-animator working-shot review remains
an HM-15 gate.

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
