# Historical foundation backlog

Preserved from the original foundation-first plan. These IDs and unclosed acceptance remain useful, but this sequence is superseded by [the character-first next tasks](FIRST-STEPS.md). Existing implementation evidence must be checked before redoing any task. Each issue is to be split further only if its acceptance cannot be demonstrated within an iteration. Stable issue IDs are local planning identifiers, not GitHub issues already created.

## P00 — feasibility issues

| ID | Task | Prerequisite | Observable evidence |
|---|---|---|---|
| BOOT-001 | Register hardware, OS, GPU/driver, tablet, compiler and benchmark recording format | None | Reproducible device matrix with tested/untested states and redistributable fixture sources |
| BOOT-002 | Audit reuse of a pinned OpenToonz checkout and selected components; assess clean build and coupling | BOOT-001 | Build log, component/license map and timeboxed adopt/reject decision; no blanket fork |
| BOOT-003 | Capture Qt tablet events and draw a pressure-sensitive B0 stroke | BOOT-001 | Pressure/tilt/focus/DPI traces and internal plus physical latency measurements |
| BOOT-004 | Compare CPU/upload and GPU render integration with Skia/Qt candidates | BOOT-003 | Identical scene images, latency distributions, memory and build complexity; one primary renderer chosen |
| BOOT-005 | Prototype stable IDs, SQLite revisions and immutable blob publication | BOOT-001 | Fault injection at each save boundary, disk-full recovery and no published missing blob |
| BOOT-006 | Evaluate clipping/region topology and curve flattening against pathological drawings | BOOT-004 | Fill/erase behavior, holes and error tolerance; chosen geometry approach and remaining gaps |
| BOOT-007 | Demonstrate rational time with audio resampling/scheduling | BOOT-001 | Ten-minute sync, exact sample/frame boundary checks and explicit drift budget |
| BOOT-008 | Evaluate rest-space curve/envelope deformation and mesh quality | BOOT-006 | Checker-texture deformation fixture, extreme-pose handling and cost report |
| BOOT-009 | Evaluate QML docking and a minimal redistributable package | BOOT-004 | Restored layout, keyboard/focus behavior, notices and install on clean target systems |
| BOOT-010 | Close feasibility ADRs and recalculate P01/P02 | BOOT-002 through BOOT-009 | Accepted evidence, explicit rejected candidates, pinned prototype revisions and updated effort ranges |

Timebox each investigation before it begins. BOOT-002 should initially consume no more than one focused engineer-week; if it finds a promising reusable subsystem, request a scoped follow-on issue rather than turning all of P00 into an unbounded fork study. The other experiments fit inside P00's combined 8–16 engineer-week envelope; failures trigger scope/architecture decisions before that envelope is silently extended.

## P01 — foundation issues

| ID | Task | Prerequisite | Observable evidence |
|---|---|---|---|
| BASE-001 | Create C++20 module targets, CMake presets, package locks and platform CI | BOOT-010 | Clean builds; domain cannot depend on Qt or platform adapters |
| BASE-002 | Specify IDs, coordinate spaces, rational intervals, alpha/color assumptions and command transactions | BASE-001 | Contract tests for time/identity/inverse commands and reviewed format examples |
| BASE-003 | Implement project save/reopen/recovery adapter from the accepted prototype | BASE-002 | Semantic round trip, migration fixture and injected failure recovery |
| BASE-004 | Implement QML tokens, English command/string registry and accessible shell | BASE-001 | Keyboard navigation, focus, HiDPI and monochrome screenshots on the declared matrix |
| BASE-005 | Connect transactional scene configuration and document revision snapshots | BASE-003, BASE-004 | Edit/undo/redo/save/reopen operates on one real document; stale jobs cannot overwrite newer state |
| BASE-006 | Audit the English glossary against command IDs, UI copy and first-party documentation | BASE-002 | Stable IDs retained, terminology consistent across implemented surfaces, generated views and links validated |
| BASE-007 | Package the shell and document contributor setup | BASE-005, BASE-006 | Clean-machine install, English instructions, known limitations and dependency notices |

## P02 — first useful film issues

| ID | Task | Prerequisite | Observable evidence |
|---|---|---|---|
| FILM-001 | Add drawing/exposure entities and a basic vector layer | BASE-007 | Independent drawings versus shared exposures survive save and undo |
| FILM-002 | Integrate pressure pencil, eraser, selection and stable swatches | FILM-001 | Recorded and physical tablet input edits the document; whole gestures undo atomically |
| FILM-003 | Add basic layer ordering and a minimal alpha-over scene evaluator | FILM-002 | Preview and headless PNG share evaluated geometry, transforms and alpha |
| FILM-004 | Add timeline exposure editing, simple onion skin and playback | FILM-003 | Hold/insert/delete/scrub at exact rational times without duplicating drawings accidentally |
| FILM-005 | Add still-image import and cancellable PNG sequence export | FILM-003 | Validated imports, correct frame numbering and no completed-looking partial output |
| FILM-006 | Perform WF-01 with an animator and publish the experimental package | FILM-004, FILM-005 | Two-second 24fps film, 48 PNGs, reopened project equality, English help and B0 performance report |

## Ticket template for subsequent phases

```yaml
id: P04-ISSUE-001
status: planned
phase: P04
feature_ids: [COL-003, COL-005]
owner_module: drawing-vector
outcome: Fill one intended region without modifying its neighbour.
requires: [accepted_topology_contract, reversible_document_commands]
supported_subset: Closed vector regions; raster fill tracked separately.
contract_changes: []
fixtures: [adjacent_regions, near_coincident_edges, small_gap]
acceptance:
  - Undo and redo restore semantic equality.
  - Reopen and export preserve the selected region and swatch identity.
  - Unsupported geometry produces an actionable English diagnostic.
evidence: []
known_limits: []
```

Before implementing a later phase, replace symbolic prerequisites with concrete completed issues/contracts and add its catalog criteria. Do not infer readiness merely because a numerically earlier phase exists in the plan.
