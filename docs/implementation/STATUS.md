# Implementation status

**Experimental editor; P11 is not complete. No phase has passed all its exit criteria.** The requested target remains P00–P11. Working subsets are marked `partial` in the feature catalog; the remaining scope has not been removed or silently deferred.

The editor runs locally on macOS and supports mouse drawing, sampled-pressure input, vector editing, layers, exposures, transform keys, project revisions and PNG export. See the [build instructions](BUILD.md) and [user guide](USER-GUIDE.md).

## Phase coverage

| Phase | Status | Available subset | Remaining before completion |
|---|---|---|---|
| P00 | in_progress | Qt-free core, CPU reference renderer, locked infrastructure dependencies, bounded OpenToonz source audit and save failure tests. | Renderer comparison, successful reuse extraction/build audit, fill/deformation spikes, device matrix and production budgets. |
| P01 | in_progress | Native English QML shell, CMake modules, commands, immutable snapshots, undo/redo and validation. | Docking/workspace persistence, configurable shortcuts, full accessibility, scalable resource/cache protocols and cross-platform qualification. |
| P02 | in_progress | Mouse pencil, exposures, onion skin, playback, save/reopen and 48-frame 1080p PNG export. | Full phase feature acceptance and animator-led workflow checks; large-project behavior and all fixture gates. |
| P03 | in_progress | Synchronized virtualized timeline/Xsheet, create/hold/clear exposures, insert/remove frames and navigate drawings. | Range selection/clipboard, drag/retime operations, annotations/thumbnails and advanced paperless workflows. |
| P04 | in_progress | Sampled vector strokes, approximate eraser, primitives, single-stroke selection, point editing/smoothing, stable palette IDs and shape recoloring. | Region topology/fill, analytic erasing, Bezier/contour/width editors, presets/textures/guides, robust selection and vector optimization. |
| P05 | planned | Single still-image import is available as a prerequisite only. | Raster tile engine, MyPaint integration, textured brushes, raster selection/transforms, image resource storage and benchmarks. |
| P06 | in_progress | Layer transform keys with linear/held/smooth interpolation and inherited transforms/opacity. | Function editor, velocity/motion paths, full key editing, cameras and multiplane evaluation. |
| P07 | planned | Exact rational frame-to-sample arithmetic has a domain test; no audio playback. | Audio decode/timeline/waveform/sync, media policies, FFmpeg adapter and video output. |
| P08 | in_progress | Parented layers, pivots, independent duplicates and linked drawing clones. | Production cut-out tools, asset/template library, substitution workflow, pose preservation, lip sync and animator validation. |
| P09 | planned | No deformation subsystem implemented. | Curve/bone/mesh deformation, constraints, binding/weights, deterministic solvers and reference fixtures. |
| P10 | planned | CPU layer painting is available; it is not a node compositor. | Typed graph evaluation, node UI, masks/effects, ROI/cache scheduling, OCIO and alpha/color reference scenes. |
| P11 | planned | Saved revisions, recovery UX, English guide, regression tests and source CI are initial prerequisites. | All required journeys, compaction/migration, hardening, supported device/OS matrix, accessibility, notices/SBOM and signed/installable releases. |

## Verification

- Locked build: 20 CTest entries pass (19 core/render cases plus two export workflows in one integration executable). Tests cover failed commands, rational time, exposure edits, hierarchy validation, rendering consistency, stale writers, schema rejection, rollback failures and abrupt process termination with a large image.
- Export integration: 48 PNGs from an immutable snapshot, rational-time metadata, cancellation and a successful subsequent job.
- Native input smoke: mouse strokes, synthetic pen pressure, cancelled gestures, undo/redo, palette validity after undo, save/reopen and a real window screenshot.
- Synthetic two-second fixture: 48 PNG frames at 1920 × 1080; reopened document is semantically identical. Initial CPU export measured 3,077 ms on this Mac; it is not a large-scene or pen-latency benchmark.
- Address/undefined-behavior sanitizers: 15/15 Qt-free core tests pass (26.86 seconds with the large crash fixture).
- The physical tablet matrix is pending because the owner has no tablet currently. Tilt/eraser-end behavior is not implemented. Mouse input works independently.
- [CI run 35528441504](https://github.com/mijim/OPEN-TOON/actions/runs/35528441504) passes for source commit `982f88f`: Windows Server 2022, macOS 14 and Ubuntu 24.04 build and test with Qt 6.8.3, plus Linux core sanitizers. Native mouse/synthetic-pen UI smoke runs on macOS. Windows process-termination recovery, other-platform GUI interaction, physical devices and installation remain unqualified.
- Standalone bundle deployment with split Homebrew Qt failed plugin/framework resolution. The build-tree application runs. No signed/notarized installer or clean-machine compatibility is claimed.

## Engineering boundaries

The domain and application layers contain no Qt. Commands validate candidates before publishing snapshots; preview/export share evaluation. [ADR-011](../architecture/adr/011-experimental-desktop-slice.md) documents the temporary CPU renderer and complete-revision SQLite storage. [Dependencies](DEPENDENCIES.md) distinguishes actual reuse from the roadmap shortlist.

Current limits include full-document copies for edits, a 64 MiB JSON revision limit, synchronous autosave, no project compaction, no exact fill topology and no managed color. These prevent a reliable P11 release claim.

## Continue toward P11

Close the open feasibility/foundation gates, qualify the first-film workflow, then finish P03/P04 before integrating raster brushes. Follow the existing dependency order through camera, audio, rigging, deformation and compositing. Do not substitute buttons or empty module scaffolding for implemented behavior. Device testing can remain pending while independent engineering proceeds.

Machine-readable source: [status.json](status.json). The catalog and roadmap retain their stable IDs and complete acceptance requirements.
