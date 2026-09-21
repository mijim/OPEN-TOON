# Open-source dependency register

> Generated from `libraries.json`. Actual experimental adoption is recorded below; see [build evidence](../implementation/DEPENDENCIES.md).

The user approved C++20 + Qt 6/QML and the reuse of proven, efficient open-source libraries. `experimental_adopted` means used in the bounded prototype, with production gates still open. `selected_for_plan` means intended adoption subject to pinned-version checks; `candidate_requires_spike` means no adoption decision yet. Upstream maturity does not prove performance in OPEN-TOON.

Record exact source revision, package checksum, build flags, enabled modules, license files, transitive dependencies and benchmark evidence when adopting a library. Each adapter must have one accountable owner. Model/brush/font/fixture licenses are separate from library licenses.

## LIB-QT — Qt 6 / Qt Quick

**Phase:** P01. **Decision:** `experimental_adopted`.

**Delivery:** harmony_moment_boundary; slices: HM-02, HM-04.

Desktop UI, tablet events, accessibility and platform integration.

**Boundary:** QML visual layer only; no Qt types in domain APIs. QRhi usage is isolated and pinned to tested Qt versions.

**Evaluation:** SP-01/02/07: real tablets, focus/DPI, graphics integration and clean installations.

**Fallback:** Use one supported Qt rendering route; reconsider QML docking independently if its candidate fails.

**License investigation:** LGPL/GPL/commercial depending on module and build.

**Primary source:** [Qt 6 / Qt Quick upstream](https://doc.qt.io/qt-6/).

## LIB-SQLITE — SQLite

**Phase:** P01. **Decision:** `experimental_adopted`.

**Delivery:** harmony_moment_boundary; slices: HM-01.

Metadata, revision transactions and local indices.

**Boundary:** Project repository adapter. Format 2 commits compressed immutable resources and revision metadata in one database transaction; ADR-012 supersedes the proposed external blob protocol.

**Evaluation:** SP-03: terminate at every commit boundary, fill disk and recover reachable blobs.

**Fallback:** Keep the repository port; revise the blob protocol before changing database technology.

**License investigation:** Public domain; review bundled extensions separately.

**Primary source:** [SQLite upstream](https://sqlite.org/atomiccommit.html).

## LIB-SKIA — Skia

**Phase:** P02. **Decision:** `candidate_requires_spike`.

**Delivery:** after_harmony_moment; slices: none required for HM.

Vector/raster drawing and a possible CPU reference renderer.

**Boundary:** Skia is not an editable drawing model, rigging engine or whole compositor. Do not add a second permanent interactive renderer without evidence.

**Evaluation:** SP-02: compare CPU upload and GPU integration for tablet latency, tile updates, alpha, color and packaging. Not a blanket HM prerequisite: keep the current constrained QML workspace and reference renderer until a measured consumer blocker justifies replacement.

**Fallback:** Use the selected Qt/RHI adapter if Skia integration adds excessive copying or build cost.

**License investigation:** BSD-style; inventory third-party components.

**Primary source:** [Skia upstream](https://skia.org/docs/).

## LIB-CLIPPER2 — Clipper2

**Phase:** P04. **Decision:** `candidate_requires_spike`.

**Delivery:** after_harmony_moment; slices: none required for HM.

Polygon clipping, offsets and geometric boolean operations.

**Boundary:** Flatten curves with recorded tolerance, retain curve provenance and keep editable topology in OPEN-TOON.

**Evaluation:** SP-04: near-coincident edges, holes, self-intersections and large coordinate ranges.

**Fallback:** Evaluate another maintained polygon kernel against the same corpus; do not silently rasterize edits.

**License investigation:** BSL-1.0.

**Primary source:** [Clipper2 upstream](https://github.com/AngusJohnson/Clipper2).

## LIB-TESS — libtess2 / earcut.hpp

**Phase:** P09. **Decision:** `candidate_requires_spike`.

**Delivery:** harmony_moment_boundary; slices: HM-05.

Select one suitable polygon tessellator for the renderer or mesh preparation.

**Boundary:** Earcut is an alternative, not an additional mandatory engine; triangle quality for deformation requires further refinement.

**Evaluation:** Compare holes/degeneracies and topology failures; do not infer solver-quality meshes from fast triangulation. HM-05 first tests a regular rest mesh over transparent artwork; add a tessellator only if the fixture demonstrates a need. Clipping and triangulation do not solve skinning or texture rendering.

**Fallback:** Use renderer-native tessellation for display and a separately justified deformation mesher.

**License investigation:** Different licenses: verify chosen implementation at pinned revision.

**Primary source:** [libtess2 / earcut.hpp upstream](https://github.com/memononen/libtess2).

## LIB-MYPAINT — libmypaint

**Phase:** P05. **Decision:** `experimental_adopted`.

**Delivery:** harmony_moment_boundary; slices: HM-02.

Established pressure-sensitive raster brush engine used by several graphics applications.

**Boundary:** Integrate the C engine through our tile surface adapter; do not embed the MyPaint application or GTK. Does not provide ABR import.

**Evaluation:** B0/B3 pressure replay, latency, tile copies, alpha/color conversion and deterministic preset replay.

**Fallback:** Keep simple native brushes initially and defer advanced presets if the adapter cannot meet budgets.

**License investigation:** ISC for core; inspect dependencies and brush asset licenses.

**Primary source:** [libmypaint upstream](https://github.com/mypaint/libmypaint).

## LIB-DOCK — KDDockWidgets

**Phase:** P01. **Decision:** `candidate_requires_spike`.

**Delivery:** after_harmony_moment; slices: none required for HM.

Docking, tabbing and saved workspaces with a Qt Quick frontend.

**Boundary:** Qt Quick support must be evaluated independently of the mature Widgets path; avoid switching the entire UI for one library.

**Evaluation:** SP-00/07: nested docks, floating windows, restore, focus, HiDPI, accessibility and monochrome customization. Not a blanket HM prerequisite: keep the current constrained QML workspace and reference renderer until a measured consumer blocker justifies replacement.

**Fallback:** Deliver a constrained QML split-panel layout first if Quick support fails; advanced docking stays incomplete.

**License investigation:** GPL-2.0-only OR GPL-3.0-only / commercial; inspect chosen files.

**Primary source:** [KDDockWidgets upstream](https://github.com/KDAB/KDDockWidgets).

## LIB-AUDIO — miniaudio

**Phase:** P07. **Decision:** `candidate_requires_spike`.

**Delivery:** harmony_moment_boundary; slices: HM-10.

Cross-platform audio device playback, mixing and capture infrastructure.

**Boundary:** One real-time audio engine; FFmpeg handles media formats outside it. No allocations or document locks in the audio callback.

**Evaluation:** SP-05: 10-minute rational-rate sync, device replacement, scrub and underrun traces. HM-10 starts with the already-resolved but unlinked miniaudio package; verify its exact revision and adopt only after clock/device tests. WAV/PCM does not require the FFmpeg adapter.

**Fallback:** Evaluate Qt Multimedia behind the same audio port, choosing one engine after measurement.

**License investigation:** Public domain or MIT No Attribution; record the selected option.

**Primary source:** [miniaudio upstream](https://github.com/mackron/miniaudio).

## LIB-FFMPEG — FFmpeg

**Phase:** P07. **Decision:** `selected_for_plan`.

**Delivery:** after_harmony_moment; slices: none required for HM.

Decode imported media and encode supported movie/audio exports.

**Boundary:** Use a restricted format/codec build and bounded jobs; available codecs are an explicit runtime capability.

**Evaluation:** Known-timestamp fixtures, cancellation, corrupt input and clean-machine export tests. First follow-on output after HM: pin one redistributable review-movie profile and prove timestamps/muxing against the PNG+WAV reference; codec failure must not break the open delivery profile.

**Fallback:** Always preserve image-sequence and WAV delivery if a movie codec cannot be distributed.

**License investigation:** Build-dependent LGPL/GPL; nonfree configurations excluded from redistributable profiles.

**Primary source:** [FFmpeg upstream](https://ffmpeg.org/legal.html).

## LIB-OCIO — OpenColorIO

**Phase:** P10. **Decision:** `selected_for_plan`.

**Delivery:** after_harmony_moment; slices: none required for HM.

Scene/display/output color transformations.

**Boundary:** Define working space, premultiplied alpha boundaries and CPU/GPU transform equality; library does not choose artistic policy.

**Evaluation:** B5 charts, negative/HDR values, config changes and GPU/CPU tolerance. HM-04 requires explicit restricted linear-sRGB/legacy profiles first; full OCIO/HDR configuration is deferred, not a waiver of alpha/color correctness.

**Fallback:** A documented restricted sRGB workflow before advanced color support; never claim HDR parity.

**License investigation:** Review exact release license plus configuration and LUT licenses.

**Primary source:** [OpenColorIO upstream](https://opencolorio.org/).

## LIB-OIIO — OpenImageIO

**Phase:** P07. **Decision:** `candidate_requires_spike`.

**Delivery:** after_harmony_moment; slices: none required for HM.

Professional image IO and metadata conversion.

**Boundary:** Start with only required plugins; generic PSD pixels do not guarantee preservation of editable PSD layers.

**Evaluation:** PNG/TIFF/EXR fixtures, resource limits, metadata/color preservation and package size.

**Fallback:** Use Qt image IO for the first PNG slice; add professional formats only when their adapters pass.

**License investigation:** Review exact release and enabled image codec dependencies.

**Primary source:** [OpenImageIO upstream](https://openimageio.readthedocs.io/en/stable/).

## LIB-EXR — OpenEXR

**Phase:** P10. **Decision:** `candidate_requires_spike`.

**Delivery:** after_harmony_moment; slices: none required for HM.

HDR channels and professional image-sequence delivery.

**Boundary:** Prefer the OpenImageIO path if sufficient; avoid a duplicate reader unless channel/deep-data needs justify it.

**Evaluation:** B5 half/float values, channels, alpha and round-trip metadata.

**Fallback:** Defer unsupported channel models with an explicit export report.

**License investigation:** Review pinned release and transitive licenses.

**Primary source:** [OpenEXR upstream](https://openexr.com/).

## LIB-OTIO — OpenTimelineIO

**Phase:** P16. **Decision:** `candidate_requires_spike`.

**Delivery:** after_harmony_moment; slices: none required for HM.

Editorial timeline interchange and rational timing.

**Boundary:** Map clips/tracks/ranges explicitly; not an animation project interchange format or media renderer.

**Evaluation:** Nested rates, gaps, missing media and round trips against specified adapter versions.

**Fallback:** Documented image/audio sequence exchange with timing manifests.

**License investigation:** Review core and adapter licenses separately.

**Primary source:** [OpenTimelineIO upstream](https://opentimelineio.readthedocs.io/en/latest/).

## LIB-OFX — OpenFX

**Phase:** P15. **Decision:** `candidate_requires_spike`.

**Delivery:** after_harmony_moment; slices: none required for HM.

Interface specification and support code for third-party effects.

**Boundary:** Implement declared host suites and process isolation; the API does not supply effects or make arbitrary plugins compatible.

**Evaluation:** Supported suite tests, crash recovery, ROI/tiles/color and plugin-specific compatibility.

**Fallback:** Keep native operators usable and mark unsupported plugins clearly.

**License investigation:** Review host headers, sample code and each external plugin separately.

**Primary source:** [OpenFX upstream](https://github.com/AcademySoftwareFoundation/openfx).

## LIB-EIGEN — Eigen

**Phase:** P09. **Decision:** `selected_for_plan`.

**Delivery:** harmony_moment_boundary; slices: HM-05, HM-06, HM-09.

Linear algebra and numerical computations for geometry and rig solvers.

**Boundary:** Use behind algorithm boundaries; avoid leaking expression-template types across public ABI.

**Evaluation:** Conditioning, degenerate transforms, precision, vectorization and compile cost. HM-05/HM-06: use only required algebra through a narrow adapter; no blanket libigl adoption or assumed mesh renderer.

**Fallback:** Use small explicit math types for simple operations; restrict solver complexity until proven.

**License investigation:** Primarily MPL-2.0; use eligible modules and record any optional dependencies.

**Primary source:** [Eigen upstream](https://libeigen.gitlab.io/).

## LIB-LIBIGL — libigl

**Phase:** P13. **Decision:** `candidate_requires_spike`.

**Delivery:** after_harmony_moment; slices: none required for HM.

Selected mesh/geometry processing routines for advanced deformation.

**Boundary:** Enable only necessary modules; no blanket CGAL/TetGen dependency or assumption of a ready-made product-specific deformer.

**Evaluation:** SP-06 extended: texture distortion, weight quality, memory and extreme poses.

**Fallback:** Ship proven bone/curve/envelope tools while advanced solvers remain partial.

**License investigation:** Primarily MPL-2.0; optional third-party modules carry other terms.

**Primary source:** [libigl upstream](https://libigl.github.io/).

## LIB-PYBIND — pybind11

**Phase:** P12. **Decision:** `selected_for_plan`.

**Delivery:** after_harmony_moment; slices: none required for HM.

Python bindings over the stable command/query API.

**Boundary:** Bindings do not sandbox Python and must not expose mutable document internals.

**Evaluation:** Ownership/lifetime, exceptions, version negotiation and worker cancellation.

**Fallback:** CLI manifest automation remains available if embedded scripting is deferred.

**License investigation:** BSD-style; confirm pinned release.

**Primary source:** [pybind11 upstream](https://github.com/pybind/pybind11).

## LIB-LIPSYNC — Rhubarb Lip Sync

**Phase:** P07. **Decision:** `candidate_requires_spike`.

**Delivery:** after_harmony_moment; slices: none required for HM.

Optional offline mouth-timing proposals after manual mouth mapping/correction works; not required for the Harmony Moment.

**Boundary:** Subprocess adapter with editable results; default speech recognizer is English-oriented, phonetic mode requires separate quality evaluation.

**Evaluation:** Human review of English and Spanish dialogue, timings, noise and character mouth mappings.

**Fallback:** Manual lip sync and import of a documented timing file.

**License investigation:** MIT core; inspect recognizer/model dependencies.

**Primary source:** [Rhubarb Lip Sync upstream](https://github.com/DanielSWolf/rhubarb-lip-sync).

## LIB-UFBX — ufbx

**Phase:** P17. **Decision:** `candidate_requires_spike`.

**Delivery:** after_harmony_moment; slices: none required for HM.

FBX scene, hierarchy and animation import candidate.

**Boundary:** Importer only; materials, animation semantics and safe resource budgets remain our responsibility.

**Evaluation:** Unit/axis/skin/clip corpus, malformed files and allocator limits.

**Fallback:** Support an explicitly smaller glTF subset first.

**License investigation:** Verify pinned release and any bundled fixtures.

**Primary source:** [ufbx upstream](https://github.com/ufbx/ufbx).

## LIB-GLTF — fastgltf

**Phase:** P17. **Decision:** `candidate_requires_spike`.

**Delivery:** after_harmony_moment; slices: none required for HM.

Modern C++ glTF parser candidate.

**Boundary:** Map assets into our 3D scene contract; parsing does not supply renderer/material support.

**Evaluation:** External URI handling, extensions, missing buffers, animation and memory limits.

**Fallback:** Restrict supported glTF extensions and provide actionable import reports.

**License investigation:** Verify pinned release and transitive dependencies.

**Primary source:** [fastgltf upstream](https://github.com/spnda/fastgltf).

## LIB-ONNX — ONNX Runtime

**Phase:** P20. **Decision:** `candidate_requires_spike`.

**Delivery:** after_harmony_moment; slices: none required for HM.

Optional local inference runtime for approved assistive tools.

**Boundary:** Not a source of trained models; each operation needs a compatible, redistributable model and separate quality evidence.

**Evaluation:** Model conversion, CPU/GPU fallbacks, installation size, latency, peak memory and cancellation.

**Fallback:** Keep AI optional; unsupported models remain unavailable without affecting normal editing.

**License investigation:** Audit runtime, execution providers and each model independently.

**Primary source:** [ONNX Runtime upstream](https://onnxruntime.ai/docs/).

## LIB-JSON — nlohmann/json

**Phase:** P01. **Decision:** `experimental_adopted`.

**Delivery:** harmony_moment_boundary; slices: HM-01.

Readable manifests, configuration and interchange metadata.

**Boundary:** Bound input depth/size; do not put JSON serialization in tablet sample or render hot paths.

**Evaluation:** Schema migration, malformed input and metadata size.

**Fallback:** Keep hot-path snapshots strongly typed.

**License investigation:** MIT.

**Primary source:** [nlohmann/json upstream](https://github.com/nlohmann/json).

## LIB-ZSTD — Zstandard

**Phase:** P05. **Decision:** `experimental_adopted`.

**Delivery:** harmony_moment_boundary; slices: HM-01.

Compress tiles, undo snapshots and packaged resources.

**Boundary:** Compression policy belongs to storage; bounded decompression and content hashes are still required.

**Evaluation:** Compression ratio versus edit latency and memory on B3/B6.

**Fallback:** Store small/hot resources uncompressed if compression harms responsiveness.

**License investigation:** BSD or GPL options; record chosen eligible license.

**Primary source:** [Zstandard upstream](https://github.com/facebook/zstd).

## LIB-ARCHIVE — libarchive

**Phase:** P08. **Decision:** `candidate_requires_spike`.

**Delivery:** after_harmony_moment; slices: none required for HM.

Portable project/library package transport.

**Boundary:** Whitelist formats; prevent path traversal, symlink escape, archive bombs and unsafe overwrite.

**Evaluation:** Unicode paths, duplicate entries, size limits and hostile archives.

**Fallback:** Use folder-based local projects until safe package transport is ready. HM-09 uses bounded folder manifests with dependency closure; compressed transport is not required for reusable rigs.

**License investigation:** Inspect BSD-style core and optional format dependencies.

**Primary source:** [libarchive upstream](https://github.com/libarchive/libarchive).

## LIB-CATCH — Catch2

**Phase:** P01. **Decision:** `experimental_adopted`.

**Delivery:** after_harmony_moment; slices: none required for HM.

Domain and adapter regression tests.

**Boundary:** Test behavior/invariants rather than mirroring implementation; UI and artistic review need separate evidence.

**Evaluation:** Run deterministic fixtures and fault injection in CI.

**Fallback:** Do not introduce a second C++ unit-test framework without a clear need.

**License investigation:** BSL-1.0.

**Primary source:** [Catch2 upstream](https://github.com/CatchOrg/Catch2).

## LIB-BENCH — Google Benchmark

**Phase:** P00. **Decision:** `selected_for_plan`.

**Delivery:** after_harmony_moment; slices: none required for HM.

Repeatable isolated CPU benchmarks.

**Boundary:** Microbenchmarks complement complete app traces; they do not measure physical pen-to-screen latency.

**Evaluation:** Fixed release build, recorded hardware, repetitions and distribution summaries.

**Fallback:** Use end-to-end frame/input traces when isolation changes the workload.

**License investigation:** Apache-2.0.

**Primary source:** [Google Benchmark upstream](https://github.com/google/benchmark).

## LIB-TRACY — Tracy

**Phase:** P00. **Decision:** `candidate_requires_spike`.

**Delivery:** after_harmony_moment; slices: none required for HM.

Developer CPU/GPU/lock/allocation profiling.

**Boundary:** Developer opt-in instrumentation only; no mandatory user telemetry or recording of artwork.

**Evaluation:** Measure instrumentation overhead and verify supported graphics backend capture.

**Fallback:** Platform profilers and local structured timing traces.

**License investigation:** Verify pinned BSD-style release.

**Primary source:** [Tracy upstream](https://github.com/wolfpld/tracy).

## LIB-BUILD — CMake / Ninja / Conan 2

**Phase:** P01. **Decision:** `experimental_adopted`.

**Delivery:** after_harmony_moment; slices: none required for HM.

Reproducible native builds, dependency graph and lockfiles.

**Boundary:** Pin exact packages/revisions and flags after spikes; never track moving latest for releases.

**Evaluation:** Clean builds and dependency restoration on every supported OS.

**Fallback:** Document a reproducible source-build recipe for dependencies missing a reliable package.

**License investigation:** Audit tools and every resolved package; package recipe is not upstream ownership.

**Primary source:** [CMake / Ninja / Conan 2 upstream](https://docs.conan.io/2/).

## LIB-JSON-C — json-c

**Phase:** P05. **Decision:** `experimental_adopted`.

**Delivery:** after_harmony_moment; slices: none required for HM.

libmypaint settings parser dependency.

**Boundary:** Private dependency of the MyPaint C adapter; document serialization continues to use nlohmann/json.

**Evaluation:** Pinned engine build and deterministic brush replay; external brush JSON imports remain unsupported.

**Fallback:** Keep original programmatic presets and validated engine adapter.

**License investigation:** MIT; notice included in third_party/licenses/json-c.

**Primary source:** [json-c upstream](https://github.com/json-c/json-c).

## LIB-OPENSSL — OpenSSL Crypto

**Phase:** P05. **Decision:** `experimental_adopted`.

**Delivery:** after_harmony_moment; slices: none required for HM.

SHA-256 resource identity and integrity verification.

**Boundary:** Private storage dependency through EVP_Digest; no TLS or network functionality. Zlib is a locked transitive dependency; NASM and Strawberry Perl are platform-specific build requirements.

**Evaluation:** Corrupt/missing resource rejection, deduplication, atomic save and 4K storage benchmark.

**Fallback:** Replace the digest adapter while retaining the SHA-256 format contract and compatibility fixtures.

**License investigation:** Apache-2.0; notices included in third_party/licenses/openssl.

**Primary source:** [OpenSSL Crypto upstream](https://github.com/openssl/openssl).

## Additional primary references

- [Earcut algorithm and its limitations](https://github.com/mapbox/earcut.hpp).
- [KDDockWidgets Qt Quick notes](https://github.com/KDAB/KDDockWidgets/blob/main/README-QtQuick.md).
- [Rhubarb licensing](https://github.com/DanielSWolf/rhubarb-lip-sync/blob/master/LICENSE.md).
- [Qt RHI compatibility](https://doc.qt.io/qt-6/qrhi.html).

Before adoption, verify recent release activity, unresolved platform blockers, security advisories, supported build tools and whether the actual workload benefits. No unpinned dependency is approved for a release merely because it appears in this register.
