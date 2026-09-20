# Architecture decision record

Proposed ADRs require evidence before becoming implementation commitments. Accepted product directions can still have unresolved backend details.

| ID | Decision | Status | Revisit when |
|---|---|---|---|
| ADR-001 | C++20 + Qt Quick desktop; Qt-free domain | Accepted by user; backend subject to spikes | SP-00/01/02 demonstrates a clearly better alternative |
| ADR-002 | Modular application, adapters and CLI sharing one core | Proposed | A real separate-deployment need appears |
| ADR-003 | Stable document IDs; drawings separate from exposures | Proposed | Fixtures reveal incompatible workflow semantics |
| ADR-004 | SQLite + immutable blobs, transactional revisions, packaged transport | Proposed | SP-03 fails recovery or cost requirements |
| ADR-005 | Renderer behind a contract; evaluate Qt RHI/Skia | Open | SP-02 completes |
| ADR-006 | Offline, open format; optional networking/AI/studio | Proposed | User changes product priorities |
| ADR-007 | GPL-3.0-or-later for original repository contributions | Adopted | Owner changes license before contributions complicate that choice |
| ADR-009 | English product, code and documentation | Accepted by user | User changes language requirements |
| ADR-010 | Reuse proven open-source components | Accepted direction | A candidate fails the documented workload or adoption checks |

| ADR-011 | Bounded experimental CPU renderer and SQLite snapshot storage | Accepted for prototype | Production performance/storage gates |

See [ADR-012](012-immutable-media-and-raster.md) for the format 2 and brush contracts.

See [ADR-011](011-experimental-desktop-slice.md) for current implementation evidence and deviations.

Existing IDs are retained; removed entries are not renumbered.

## ADR-001 — Platform

The scope needs tablets, geometry, audio, large files and graphics libraries. Use Qt Quick for UI and C++20 for the domain/engine. Alternatives include Qt Widgets, Tauri/Rust, Electron with a native engine and the browser. Consequences: C++ discipline and specialized UI work, with fewer boundaries between professional libraries. See the [comparison](../01-technology-selection.md); review integration evidence after SP-00/01/02.

## ADR-004 — Saving

A single readable file is attractive, but rewriting all drawings on every save harms editing. A database alone does not guarantee durability of external files. Proposed decision: SQLite revision data plus immutable blobs published before commit. Consequences: explicit protocol, reachability-based garbage collection and separate readable export. Evidence awaits SP-03. See the [format proposal](../03-document-model.md).

## ADR-005 — Graphics backend

No final renderer is selected. QQuickRhiItem is a GPU presentation/evaluation candidate; QRhi has limited compatibility guarantees. Evaluate Skia for paths/rasterization and retain a CPU reference for independent tests. Decide after measuring latency, memory, color and integration cost. Reject any design that copies an entire serialized frame for each pen sample.

## ADR-007 — License

The repository adopts GPL-3.0-or-later for original contributions, favoring open redistributed modifications. This does not change dependency, font or user-artwork licenses. Evaluate Qt modules, FFmpeg and plugins for each actual build.

## ADR-009 — English product language

Accepted by user on 2026-09-20. All first-party UI, built-in assets, help, code and public docs use English. User content remains multilingual. Earlier research and catalogs have been translated with stable IDs in this planning update. See the [policy](../../design/02-language-policy.md) and `NFR-027`.

## ADR-010 — Reuse proven open-source components

Prefer maintained libraries for solved infrastructure through explicit adapters. Specific adoption remains conditional on workload benchmarks, pinned versions and redistribution checks. The [register](../../planning/LIBRARIES.md) separates intended choices from candidates and defines fallbacks. The original planning change installed no dependencies; current experimental adoption is recorded in `docs/implementation/DEPENDENCIES.md`.

See [ADR-013](013-animation-editing.md) for setup/animate, pose curve and key-only retiming semantics.

See [ADR-014](014-drawing-selection.md) for rectangular vector/raster selection, compositing and brush opacity.

See [ADR-015](015-direct-editing-workspace.md) for direct transform handles, integrated timing and selection-owned Properties; it supersedes the earlier dialog interaction.
