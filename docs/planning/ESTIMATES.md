# Effort, staffing and uncertainty

These are **planning judgments**, not measured implementation velocity, contractual dates or quotations. No application prototype has yet established the team's throughput. Estimates include integration, appropriate tests, documentation and ordinary iteration; they do not assume AI-generated code removes graphics research, device testing or artist review.

One focused engineer-week means five working days from one experienced engineer. Summing engineer-weeks measures labor, not calendar time. Phase estimates are incremental; shared infrastructure is budgeted in its owning phase. Later phases have much lower confidence and must be re-estimated before commitment.

| Delivery envelope | Included phases | Focused engineer-weeks | Confidence today |
|---|---|---:|---|
| First complete animation | P00–P02 | 36–66 | Low; improves after the spikes |
| Drawing, timing and media toolbox | P00–P07 | 110–198 | Low |
| Reliable offline 2D 1.0 | P00–P11 | 182–330 | Low |
| Entire current roadmap including optional branches | P00–P22 | 378–716 | Very low; order-of-magnitude envelope |

The P00–P07 cumulative row is a scope bundle, not a dependency claim: P06 and P07 can overlap with separate owners. The complete sum includes AI, legacy, studio and 3D branches that can be rescheduled or explicitly deferred. It excludes indefinite maintenance after P22. Phase-level ranges are in [PHASES.md](PHASES.md).

## Illustrative team scenario

A practical initial team has three experienced engineers: graphics/geometry, document/animation/persistence, and Qt/presentation/media. Add an animator/product reviewer and part-time QA/release capacity. Those roles are assumptions, not people currently assigned. Cross-review prevents a single specialist from owning an undocumented subsystem.

At **70% focused engineering capacity**, three engineers deliver approximately **2.1 engineer-weeks per calendar week**. The remaining capacity covers planning, coordination and support. Reserve another **20% schedule contingency** for unknown integration/device problems; do not call it extra functionality.

Under that simplified model, the first complete slice is roughly **21–38 calendar weeks**, and a reliable 2D 1.0 is roughly **104–189 weeks**. This illustrates the size of professional animation tooling, not a promised launch date. A team of five experienced engineers at the same capacity/contingency gives a naive 1.0 envelope of approximately **63–114 weeks**; critical dependencies and specialist availability can make it longer. Adding people does not linearly accelerate topology, rendering or architecture decisions.

A single developer should narrow the initial scope and keep advanced/studio branches uncommitted; the full roadmap is a multi-year product program. Good open-source reuse reduces the amount of infrastructure we own, but cannot remove product integration or animation semantics. We should use actual P00/P02 evidence to replace these ranges, rather than invent an optimistic date now.

## Recalibration gates

- **After P00:** choose renderer, brush/docking candidates and format protocol; update library adoption costs and foundational estimates.
- **After P02:** use actual complete workflow throughput, tablet results and storage stability to estimate the paperless/vector releases.
- **After P07:** use user feedback and mixed-media performance to resize rigs and composition.
- **After P11:** commit only the next two advanced phases using production experience and staffing.
- **Before each optional extension:** verify a real user workflow, viable dependency/model license and a maintenance owner.

Track completed end-to-end capabilities, reopened/rendered fixtures and regressions; avoid counting buttons, lines of code or closed micro-tickets as velocity. Publish revised ranges with their assumptions when evidence changes. If a deadline becomes a requirement, negotiate a smaller measurable scope instead of silently dropping recovery, English UI or data integrity.
