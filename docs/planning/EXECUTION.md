# Engineering execution and release rules

## Module boundaries

Use one modular desktop application and a shared headless core. The planned target boundaries are:

| Layer / target family | Owns | Must not own |
|---|---|---|
| `core/time`, `core/geometry`, `core/document` | Stable IDs, drawings, exposure intervals, transforms, rational time, document invariants | QML, windows, platform IO or network services |
| `application/commands`, `application/jobs` | Transactions, undo/redo, validated use cases, snapshots and cancellable jobs | Renderer internals or direct widget manipulation |
| `engine/drawing`, `engine/animation`, `engine/compositing` | Geometry, evaluation, deformation, graph execution and cache keys | Mutable UI state or ad hoc persistence |
| `adapters/storage`, `adapters/media`, `adapters/graphics` | SQLite/blob protocol, codecs, renderer/library integration | Independent interpretations of time or scene semantics |
| `adapters/input`, `adapters/scripting` | OS events, tablet normalization and versioned scripting boundary | Bypassing commands to mutate documents |
| `desktop/qml`, `desktop/presentation` | English strings, panels, tool interaction, focus and view models | A second copy of the document model |
| `cli`, `studio`, `workers` | Headless jobs and optional isolated integrations | A different evaluator or mandatory cloud dependency |

Names are proposed module targets, not a requirement to create empty directories before they have a consumer. C++20, explicit ownership, RAII, immutable evaluation snapshots and bounded queues are defaults. Avoid inheritance trees where a small interface or value type suffices. Dependency rules are enforced in the build graph and focused architecture checks, not by comments alone.

Keep Qt's visual scene graph separate from the animation document and compositor graph. Keep UI selection/hover separate from saved artwork. Each command declares affected IDs and invalidation scope. Background rendering consumes a revision snapshot and publishes only if its result still matches the requesting revision.

## Contracts defined before extension

- **Coordinates:** drawing, local, parent, scene, camera and device spaces; units, handedness, pivots and negative scales.
- **Time:** rational frame rate and sample rate, half-open intervals, rounding at media boundaries and one authoritative playback clock.
- **Color:** working-space assumptions, premultiplied-alpha boundaries, brush surface conversion and final display/output transforms.
- **Persistence:** schema versions, stable IDs, immutable blob hashes, publish-before-reference protocol, recovery and garbage collection reachability.
- **Evaluation:** deterministic input snapshot, revision-keyed caches, parameter sampling, invalidation, cancellation and memory budgets.
- **Extensions:** API/version negotiation, unsupported-feature reporting, explicit trust and permissions, worker failure recovery.

The first implementation may support a limited profile, but its data model must not silently equate these concepts. Update the corresponding ADR before changing a persisted or public contract.

## Reuse before new infrastructure

For each dependency decision: identify the required behavior; compare the existing library with a small owned implementation; record maintenance activity, build support, license and transitive graph; benchmark the actual workload; pin the accepted source/package revision. Package-manager availability alone is not proof of suitability.

Prefer the [registered libraries](LIBRARIES.md) for solved infrastructure. Own the product-specific document semantics, command model, editing behavior, animation graph and artistic UX. Avoid whole-app forks solely to acquire one reusable component. Reuse OpenToonz components only after a bounded coupling/build/license audit and an explicit ownership decision.

Each adapter has a contract test corpus and an escape route. Add only the required modules and formats. Upstream fixes should be contributed when practical; keep unavoidable patches small and tracked. Record all licenses/notices in a generated dependency inventory and ship corresponding required materials. Do not duplicate image/audio/render engines merely because two features use different libraries.

A dependency release proposal includes changes to lockfiles, build options, golden outputs, performance baselines and migration compatibility. An update is accepted after those affected boundaries pass; unrelated suites need not run repeatedly during documentation-only edits.

## Shared Definition of Done

A capability may be marked `implemented` only when it performs the catalog behavior on a real document, persists its state, supports undo/redo where applicable, handles invalid input and cancellation, and documents unsupported cases in English. `verified` additionally requires evidence on the declared platform/device/format profile.

For each implementation ticket, retain:

1. Feature IDs, owning module, exact supported subset and prerequisites.
2. A contract or ADR delta for format, time, color, renderer or extension changes.
3. Observable user acceptance and the smallest relevant fixtures.
4. Invariant, fault-injection, image or interaction evidence appropriate to the change.
5. Performance/memory evidence when the change touches a hot path or a declared budget.
6. License/provenance records for newly shipped dependencies, models, brushes or sample assets.
7. English UI/help strings, keyboard/focus behavior and relevant accessibility evidence.
8. Migration and recovery behavior; release notes and known limitations.

A passed test that merely duplicates implementation logic is insufficient. Real tablet tests supplement replay tests. Artist review supplements pixel comparisons. Always investigate golden-image changes before accepting them.

## Cadence and release gates

Work in two-week iterations as a coordination rhythm, not a delivery guarantee. At the start, select bounded capability tickets whose prerequisites are present. At the end, demonstrate a saved/reopened/exported scene, report measured regressions and revise the next iteration. Use an animator review at each workflow milestone; each phase has its own explicit exit criteria.

Early builds cover all three desktop OSes; supported tablet/GPU configurations expand through measured profiles. Use the current macOS workstation for initial development convenience without asserting that untested Windows/Linux drivers work. A release advertises only the profiles actually validated.

CI starts with docs, deterministic domain tests and clean builds. Add meaningful render fixtures, storage failure injection and untrusted-parser tests as those systems appear. Sanitizers and platform diagnostics run where supported. Performance comparisons use recorded hardware and release builds; heterogeneous shared CI timings are not hard frame-budget evidence.

Before a release, require zero known data-loss defects and zero unresolved critical release blockers, passing migrations/recovery, install/upgrade checks, dependency notices, English help and accurate capability claims. Maintain a previous stable release path. Automated or manual release signing must use managed credentials outside the repository; account-specific distribution setup is a release prerequisite, not something to fabricate now.

## Backlog and risk handling

Phases are epics; the 69 work packages are planning units. Split them into issues small enough to demonstrate within one iteration. Do not turn the entire long roadmap into hundreds of rigid micro-tasks before the first benchmark. Re-estimate after P00, P02, P07 and P11, and whenever the renderer, format or staffing changes.

Highest early risks: tablet latency, editable vector topology, renderer/color integration and crash safety. Later risks: solver quality, OpenFX isolation, layered interchange semantics, 3D material mapping and distributed revision conflicts. Experiments that fail are valid evidence: narrow scope or change the adapter, document the decision and preserve the last working release.

## Current development verification cadence

Owner direction, 2026-09-20: develop coherent functionality blocks on macOS, then
run relevant tests at the end and fix failures iteratively. Do not rebuild after
every small edit. Current releases publish source code. The native cross-platform
workflow is manual (`workflow_dispatch`); Linux and Windows qualification and binary
packaging return for future compiled releases. Historical cross-platform results do
not qualify newly changed source. This cadence does not waive correctness or phase
acceptance; record which platform and source were actually checked.
