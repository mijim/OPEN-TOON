# Quality and feasibility experiments

The [27 nonfunctional requirements](../catalog/nonfunctional.json) are proposed objectives, **not measured results**. Animation software is validated through drawings, timing, persistence and devices, not only UI component tests.

## Proposed load fixtures

| Scene | Synthetic content | Purpose |
|---|---|---|
| B0 | One layer/drawing, recorded tablet strokes, 1080p | Input latency and fidelity |
| B1 | 100 layers, 1,000 frames, 200 unique drawings, 100,000 total vector segments, one 48 kHz WAV track, basic masks | Initial UI, cache and playback budgets |
| B2 | 300 layers, 10,000 frames, 1,000 drawings, curves and reused rigs | Virtualization and invalidation |
| B3 | 4K sparse raster, textures, blur, masks and transparency | Memory, tiles and quality |
| B4 | Curve/envelope rig, 60 controls, substitutions and poses | Deformation and controllers |
| B5 | HDR, color charts, alpha, EXR and multiple outputs | Compositor consistency |
| B6 | Corrupt projects, missing resources and old versions | Recovery and parsers |

Generate fixtures with fixed seeds, record hashes and distinguish total from visible segment counts. Before testing, record CPU, GPU, RAM/VRAM, OS, driver, tablet, refresh rate, DPI, resolution and release build. A 16 GiB machine is a budget reference, not an already-validated minimum requirement. Early phases exercise the explicitly supported subset of each fixture.

## Technical gates

| ID | Uncertainty | Bounded experiment | Decision evidence |
|---|---|---|---|
| SP-00 | Owned core versus OpenToonz reuse/fork | Audit saving, exposures, vector/raster, Plastic and compositor in an identified checkout; build and change a UI element | Coupling/license map and reuse versus development cost |
| SP-01 | Qt input and pen response | Capture pressure/tilt and draw B0 on each OS with real tablets; test HiDPI, focus loss and cancellation | Internal traces, physical-latency video and driver matrix; change adapter if needed |
| SP-02 | Renderer and Qt Quick | Compare CPU/upload with GPU RHI; evaluate Skia at the vector boundary, tiles, alpha, filters and surfaces | Frame times, memory, build cost, visual equivalence; select one primary path |
| SP-03 | Robust saving | Minimal SQLite/blob prototype with injected failures between every step | Recovery without broken references and concurrent-save cost |
| SP-04 | Vector topology and painting | Self-intersections, variable width, adjacent regions, gap closing and erasing | Pathological drawing suite and error criteria; choose algorithm/library |
| SP-05 | Time model and audio | Mix integer/rational FPS, clips and rate changes over ten minutes | Exact frame counts and drift within the target |
| SP-06 | Deformation | Curve/envelope and mesh prototype on B4 with checker texture | Joint quality, continuity, performance and explicit limits |
| SP-07 | Free-software distribution | Build a minimal package with intended Qt modules, fonts and codecs | Exact dependencies/licenses, clean-system installation and plugin loading |

Particles and AI need not be solved before the first useful product. Drawing, saving and playback viability must be established. Failed spikes update the ADR and roadmap; do not hide them behind visual-design goals.

## Types of verification

**Domain:** time arithmetic, intervals, identities, curves, transforms, inverse commands and graph order. Generative edit sequences compare invariants rather than duplicating the implementation algorithm.

**Persistence:** save/open, migrations, backups, disk full, Unicode paths, interruptions and concurrent access. Require semantic equality and recoverable resources, not byte equality of databases containing timestamps.

**Images:** deterministic CPU rendering where possible and perceptual/numerical GPU comparisons with per-operator tolerances. Compare alpha separately; include edges, out-of-frame content, HDR and palette/deformation/mask combinations. Investigate golden changes before accepting them.

**UI:** keyboard navigation, selections, range operations, long text, scaling, empty states and focus recovery. Synthetic strokes catch geometry regressions but do not replace physical tablet testing. All first-party surfaces follow the [English policy](../design/02-language-policy.md).

**Performance:** separate cold/warm caches; record p50/p95/p99 and compare the same machine/build. Separate CPU/GPU/IO. Measure saving while drawing and avoid averages that hide long pauses.

**Artist workflows:** complete [WF-01 through WF-12](../research/02-workflows.md) with an animator, recording blockers, losses and confusing steps. Menu counts do not prove production readiness.

## Definition of Done

A capability moves from `not_started` to implemented when it performs its operation on the real model, persists, supports undo where applicable, explains errors and meets observable acceptance. It becomes verified after passing declared platforms/profiles and documenting limits. Partial support remains `partial` with its exact subset.

A release requires clean builds, third-party notices, a versioned format, reference scenes, upgrade/recovery checks, installable packages and a limitations guide. Advertise resolution, layer counts and codecs only after measuring them.

The [roadmap](../planning/README.md) assigns phases and release gates to this strategy. All budgets remain proposed until measured on recorded hardware.
