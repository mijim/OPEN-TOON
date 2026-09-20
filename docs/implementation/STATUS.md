# Implementation status

**Experimental editor; P11 is not complete. No phase has passed all its exit criteria.** The requested target remains P00–P11. Working subsets are marked `partial` in the feature catalog; the remaining scope has not been removed or silently deferred.

The editor runs locally on macOS and supports mouse drawing, sampled-pressure input, vector editing, layers, exposures, transform keys, project revisions and PNG export. See the [build instructions](BUILD.md) and [user guide](USER-GUIDE.md).

## Phase coverage

| Phase | Status | Available subset | Remaining before completion |
|---|---|---|---|
| P00 | in_progress | Qt-free core, CPU reference renderer, locked infrastructure dependencies, bounded OpenToonz source audit and save failure tests. | Renderer comparison, successful reuse extraction/build audit, fill/deformation spikes, device matrix and production budgets. |
| P01 | in_progress | Native English QML shell, CMake modules, commands, immutable snapshots, undo/redo and validation. Integrated resizable timing/curve workspace and selection-owned object/layer Properties. | Docking/workspace persistence, configurable shortcuts, full accessibility, scalable resource/cache protocols and cross-platform qualification. |
| P02 | in_progress | Mouse pencil, exposures, onion skin, playback, save/reopen and 48-frame 1080p PNG export. | Full phase feature acceptance and animator-led workflow checks; large-project behavior and all fixture gates. |
| P03 | in_progress | Synchronized virtualized timeline/Xsheet, create/hold/clear exposures, insert/remove frames and navigate drawings. Multi-layer range clipboard, independent drawing paste, key paste, cycles, ones/twos/threes, overwrite retiming, Alt-drag moves, markers and bounded Xsheet PDF export. | Frame annotations/thumbnails, advanced onion/tracing controls, paperless workflows and artist acceptance. |
| P04 | in_progress | Sampled vector strokes, approximate eraser, primitives, single-stroke selection, point editing/smoothing, stable palette IDs and shape recoloring. Rectangular whole-stroke multi-selection, move/duplicate/delete, flips and quarter turns preserve editable geometry, swatch and art-layer IDs. On-canvas scale/rotation handles with live previews and explicit selected-vector properties. | Region topology/fill, analytic erasing, Bezier/contour/width editors, presets/textures/guides, lasso/partial/additive selection and vector optimization. |
| P05 | in_progress | Sparse immutable 15-bit premultiplied tiles, libmypaint ink/soft/dry/smudge/eraser presets, pressure input, cancellable gestures, shared preview/export and compressed project resources. Brush opacity and rectangular pixel selection with integer move/duplicate/delete, flips and lossless quarter turns, alpha-safe overlap and guarded canvas bounds. Free scale/rotation preview and commit using nearest-neighbor sampling with transparent surroundings. | Brush texture/import/preset management, lasso/soft masks, high-quality resampling and transform performance qualification, resolution changes, production workload and physical-device qualification. |
| P06 | in_progress | Layer transform keys with linear/held/smooth interpolation, inherited transforms/opacity, explicit Setup/Animate and Auto key, key navigation/state, editable pose-channel graph and key-only multi-layer retiming with collision rejection. Curves stay in the main workspace; timeline/Xsheet range-end dragging stretches timing. | Independent channel keys, editable tangents, velocity/motion paths, complete batched editing, cameras and multiplane evaluation. |
| P07 | planned | Exact rational frame-to-sample arithmetic has a domain test; no audio playback. | Audio decode/timeline/waveform/sync, media policies, FFmpeg adapter and video output. |
| P08 | in_progress | Parented layers, pivots, independent duplicates and linked drawing clones. | Production cut-out tools, asset/template library, substitution workflow, pose preservation, lip sync and animator validation. |
| P09 | planned | No deformation subsystem implemented. | Curve/bone/mesh deformation, constraints, binding/weights, deterministic solvers and reference fixtures. |
| P10 | planned | CPU layer painting is available; it is not a node compositor. | Typed graph evaluation, node UI, masks/effects, ROI/cache scheduling, OCIO and alpha/color reference scenes. |
| P11 | planned | Saved revisions, recovery UX, English guide, regression tests and source CI are initial prerequisites. Format 1 migration backups, checked format 2 resources, explicit backed-up compaction and asynchronous recovery snapshots. | All required journeys, remaining hardening, supported device/OS matrix, accessibility, complete notices/SBOM and signed/installable releases. |

## Verification

- Current macOS locked build: 43/43 CTest entries pass (42 core/render cases and five integration cases in one executable). Tests cover failed commands, rational time, exposure edits, hierarchy validation, rendering consistency, stale writers, schema rejection, rollback failures and abrupt process termination with a large image.
- Export integration: 48 PNGs from an immutable snapshot, rational-time metadata, cancellation and a successful subsequent job.
- Native input smoke: mouse strokes, synthetic pen pressure, cancelled gestures, undo/redo, palette validity after undo, save/reopen and a real window screenshot.
- Synthetic two-second fixture: 48 PNG frames at 1920 × 1080; reopened document is semantically identical. Initial CPU export measured 3,077 ms on this Mac; it is not a large-scene or pen-latency benchmark.
- Historical experimental.1 address/undefined-behavior sanitizers: 27/27 Qt-free core/brush tests pass with locked Debug dependencies (4.35 seconds).
- The physical tablet matrix is pending because the owner has no tablet currently. Tilt is routed to MyPaint; built-in presets do not use tilt mappings. Eraser-end behavior is not implemented. Mouse input works independently.
- [CI run 35531337766](https://github.com/mijim/OPEN-TOON/actions/runs/35531337766) passes for source commit `c07414f`: Windows Server 2022, macOS 14 and Ubuntu 24.04 build and test with Qt 6.8.3, plus Linux core sanitizers. Native mouse/synthetic-pen, raster save/reopen and range-drag UI smoke pass on macOS. Windows process-termination recovery, other-platform GUI interaction, physical devices and installation remain unqualified.
- Standalone bundle deployment with split Homebrew Qt failed plugin/framework resolution. The build-tree application runs. No signed/notarized installer or clean-machine compatibility is claimed.

The [4K brush benchmark](BRUSH-BENCHMARK.md) records measured engine and storage costs. Native input smoke covers raster painting, cancellation, undo/redo, pixel-identical reopening, range clipboard and actual range selection/Alt-drag events. Five consecutive local runs passed.

## Engineering boundaries

The domain and application layers contain no Qt. Commands validate candidates before publishing snapshots; preview/export share evaluation. [ADR-011](../architecture/adr/011-experimental-desktop-slice.md) documents the temporary CPU renderer and complete-revision SQLite storage. [Dependencies](DEPENDENCIES.md) distinguishes actual reuse from the roadmap shortlist.

[ADR-012](../architecture/adr/012-immutable-media-and-raster.md) adds immutable media, compressed/checksummed resources, migration backups, compaction and background recovery. Current limits include copied vector metadata for edits, a 64 MiB metadata limit, synchronous manual save, no exact fill topology and no managed color. These prevent a reliable P11 release claim.

## Continue toward P11

Close the open feasibility/foundation gates, qualify the first-film workflow, then finish P03/P04 acceptance and qualify the experimental P05 raster adapter. Follow the existing dependency order through camera, audio, rigging, deformation and compositing. Do not substitute buttons or empty module scaffolding for implemented behavior. Device testing can remain pending while independent engineering proceeds.

Machine-readable source: [status.json](status.json). The catalog and roadmap retain their stable IDs and complete acceptance requirements.

## Animation editing source release

Experimental.2 adds explicit rest/animation editing, guarded autokey, key navigation,
a sampled pose-curve editor with numeric and drag edits, and collision-safe key-only
retiming. See [ADR-013](../architecture/adr/013-animation-editing.md) for exact limits.
Current verification is macOS only. Native UI smoke uses the integrated curve panel and checks
an actual key drag and undo. The full cross-platform workflow is now manual, and
source releases do not produce installers. Historical OS/sanitizer results above
refer to their recorded source, not automatic qualification of new animation code.

## P04/P05 drawing selection source release

Experimental.3 focuses on whole-stroke multi-selection and sparse raster selection:
move, duplicate, delete, flip and lossless clockwise quarter turns. Raster opacity
is exposed, and erasing ignores selected swatch alpha. The native smoke checks actual
marquee/move gestures, cancellation, undo/redo and pixel-identical save/reopen. Four
core selection cases cover tile overlap, alpha, shared buffers, vector identity and
atomic rejection. See [ADR-014](../architecture/adr/014-drawing-selection.md).

These are working subsets, not completed P04/P05. Region fill topology, advanced
vector geometry, lasso/masks, high-quality raster resampling and brush import remain open.

## Direct editing workspace correction

Experimental.4 removes the separate selection, curve and timing dialogs. Canvas
transform handles move/scale/rotate with live artwork previews; Properties follows
the selected stroke/region or explicitly selected layer. Curves and timing tools
stay in the resizable lower workspace, and dragging a range end stretches timing.
Native tests exercise these gestures, preview atomicity, selection preservation,
Properties updates and screenshots. Visual inspection caught and fixed a Retina
overlay coordinate mismatch. See [ADR-015](../architecture/adr/015-direct-editing-workspace.md)
for resampling/primitive-conversion limits. One startup input attempt failed with
another development instance running; the isolated retry passed.
