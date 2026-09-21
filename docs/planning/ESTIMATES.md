# Effort, staffing and uncertainty

These are planning judgments, not delivery dates or measured velocity. The editor
exists, but the team has not demonstrated a complete deformable-character workflow.
One engineer-week means five focused working days; it is not a calendar week at full
availability. Include integration, failure handling, appropriate tests, documentation
and artist review in each estimate.

## Two different envelopes

| Envelope | Meaning | Engineer-weeks | Confidence |
|---|---|---:|---|
| Harmony Moment | Remaining bounded work in HM-00–HM-15 from experimental.10 | 47–92 | Low; deformation/render integration is the main unknown |
| Dependency-only longest HM chain | Serial work with unlimited independent staffing; excludes resource contention | 23–45 | Low; not an elapsed-time promise |
| Whole long-term program | Full scope of all 23 phases, including original work and optional branches | 381–722 | Very low; not remaining work |

`roadmap.py stats` computes the HM sum and dependency-only longest paths from canonical
slice estimates. Those slices are subsets of the phase envelopes: **never add the two
budgets**. Earlier estimates were whole-scope envelopes, not a reliable measurement
of unfinished work. Source/test counts do not establish graphics or animator throughput.

## Reallocation versus added scope

The prior whole-program envelope was 378–716. This revision transfers 6–10 weeks of
declarative controls/guides from P13 to P08, 2–4 of core IK from P13 to P09, 2–4 of
scripted controls from P13 to P12, and 2–4 of lipsync from P08 to P07. These transfers
are not added work. The distinct Quick Rig requirement adds 3–6 to P08, producing
381–722 overall. Advanced constraints, pose grids and solver breadth stay in P13.

The 47–92 HM range estimates remaining work in the narrowed profile, with each slice
charged once to its owner. Quick Rig's first FK recipe is smaller than its complete
catalog requirement. Existing drawing/storage/animation reduce baseline work but
must be qualified for new consumers. If a deformation renderer replacement becomes
necessary, revise its owning slice before committing to the new cost; the estimate
does not hide an unlimited rewrite.

## Capacity and scheduling

The estimated longest chain is HM-00 → HM-01 → HM-03 → HM-05 → HM-06 → HM-09 → HM-15.
Other parents, notably the graph/evaluation foundation, also gate joins. The path can
change with actual measurements; parallel branches are not free work.

For one engineer, the labor sum dominates. With two distinct owners, parallelize the
character/deformation path and audio or control/composition work, with one integration
owner and no more than two active slices. Never claim a twofold speedup: shared
schemas, graphics review, artist feedback and integration constrain concurrency.
No staffing or release calendar has been promised.

## Recalibration gates

- HM-00: record current host, real fixture workload, acceptance budgets and evidence.
- HM-01: measure migration/property integration cost and revise dependent slices.
- HM-05: prove bind-to-render deformation quality/cost; adopt/reject the backend and
  revise estimates before building its editor.
- HM-10: measure device clock, drift, scrub and mixing; revisit the media adapter if needed.
- HM-09/HM-15: measure reusable-rig closure and independent animator completion, then
  schedule movie export and the highest-value retained follow-ons.
- Before any broader platform, solver, parser, codec or optional extension: verify a
  bounded profile, dependency provenance and a maintenance owner.

Physical tablets are not currently available. Synthetic input and mouse workflows
remain useful evidence, but they cannot qualify tablet latency or driver behavior.
Data integrity and a working deformation renderer cannot be waived to meet a date.
