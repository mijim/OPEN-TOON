# Technology selection

## Accepted direction

Build a **native desktop application with C++20, Qt 6 and Qt Quick/QML**, keeping domain libraries independent of Qt. Target Windows, macOS and Linux; validate one platform first while building and checking the others as soon as a usable core exists. Web distribution is not a user requirement and should not constrain the engine.

The user accepted this direction and explicitly requested proven open-source reuse on 2026-09-20. Individual renderer, docking and integration decisions still require technical evidence.

## Criteria

| Criterion | Indicative weight | Why it matters |
|---|---:|---|
| Pen input and desktop integration | 25% | Pressure, tilt, capture, HiDPI and multiple monitors |
| Graphics engine and computation | 25% | Complex geometry, raster, deformation and compositing |
| Professional interoperability | 15% | C/C++ libraries, audio, images, color, codecs and plugins |
| Maintainability and extensibility | 15% | Domain separation, tests and contributions |
| UI development and productivity | 10% | Editor density, accessibility and custom design |
| Portability and distribution | 10% | Three operating systems and installable dependencies |

These weights are proposed for this product. No numerical scores imply measurements that have not occurred.

## Alternatives

| Option | Benefits for OPEN-TOON | Costs and risks | Assessment |
|---|---|---|---|
| C++20 + Qt Quick | Native events, graphics ecosystem, direct interoperability, customizable declarative UI | Memory discipline; advanced docking and animation controls require work; demanding GPU integration | Selected direction |
| C++20 + Qt Widgets | Established desktop controls/docking; pragmatic for Qt teams | Fine styling and visual transitions require more manual work; widgets do not solve the graphics engine | Alternative if QML docking/accessibility fails |
| Rust + Tauri + React | Rust domain, productive web UI, relatively small shell | Webview differences; pen/render data transfer needs design; C++ SDK hosting adds FFI | Viable for a web-focused team if spikes pass |
| Electron + React + native core | Consistent Chromium, web UI and diagnostics | Larger memory/distribution footprint; native boundary and runtime/module versioning | Web alternative, not preferred for this base |
| Browser + WebGPU/WASM | No installation and easy link sharing | Browser/filesystem/codec/worker limits; less tablet-environment control | Future viewer/reviewer, not the initial primary editor |
| Rust + wgpu + native UI | Ownership control and portable graphics abstraction | Desktop artist workflows and DCC integration need additional work | Good engine alternative; do not add a second language without need |
| Python + PySide | Accessible UI prototyping and technical ecosystem | Heavy workloads need native code; two languages and packaging complexity | Pipeline tools/prototypes, not the render core |

[Qt provides pressure and tilt tablet events](https://doc.qt.io/qt-6/qtabletevent.html), but real driver/device combinations still need testing. [Qt Quick controls are customizable](https://doc.qt.io/qt-6/qtquickcontrols-customize.html), so a Geist-inspired style does not require React. [Tauri uses webview/Rust IPC](https://v2.tauri.app/concept/inter-process-communication/), [Electron uses Chromium's process model](https://www.electronjs.org/docs/latest/tutorial/process-model), and [wgpu provides a portable graphics abstraction](https://wgpu.rs/). These are documented properties; the product assessments are project-specific judgments.

## Build, reuse or fork

Existing free-software alternatives deserve evaluation. [OpenToonz](https://opentoonz.github.io/e/) provides raster/vector workflows, exposure, compositing and effects; its [Plastic tool](https://opentoonz.readthedocs.io/en/latest/create_animations_using_plastic_tool.html) uses mesh/skeleton deformation. A fork could accelerate some functionality, but adapting models, UI and architecture costs effort and reduces design control. A BSD license on its main code does not establish the license of every dependency or asset.

[Krita](https://docs.krita.org/en/user_manual/animation.html) is a useful painting and raster-animation reference; its manual distinguishes that focus from extensive audiovisual editing. [Blender Grease Pencil](https://docs.blender.org/manual/en/4.2/grease_pencil/animation/introduction.html) combines animated drawing and 3D space, within a different overall product model.

**Proposal:** own the product core, reuse mature general-purpose libraries and investigate specific components before rewriting algorithms. P00 includes SP-00, a bounded OpenToonz audit covering reuse value and coupling. Its code has not yet been audited here; avoiding a whole-app fork remains provisional. Reconsider adaptation if staffing or deadlines make the owned scope infeasible.

## Stack by responsibility

| Responsibility | Proposal | Condition |
|---|---|---|
| Domain, commands, evaluation | C++20, STL, RAII, value types | No Qt or application globals in the domain |
| UI | Qt 6 Quick/QML; custom controls over Basic | Model/view separation, accessible navigation, no document logic in QML |
| Vector engine | Owned geometry; Skia rasterization/path candidate | Evaluate regions, coverage, alpha and editable-data preservation |
| GPU backend | Qt RHI adapter; experimental Skia surface integration | Do not assume zero-copy interoperability |
| Color | OpenColorIO with versioned production configuration | Separate working and display transforms |
| Images | PNG/EXR adapters; OpenImageIO candidate | Review actual included codecs/licenses |
| Audio/encoding | FFmpeg process/adapter and bounded audio engine | Build-specific licenses, audio clock, cancellation, explicit profiles |
| Local project | SQLite metadata/revisions plus immutable hashed blobs | Commit design spans files and database, not just SQL |
| Editorial interchange | Optional OpenTimelineIO adapter | Does not carry complete rig/compositor semantics |
| Automation | Shared-core CLI and bounded Python API later | Essential editing never depends on scripts |
| Plugins | Internal API first; OpenFX adapter later | Defined host profile, processes and versions |
| Build | CMake presets, Ninja, Conan 2 | Pinned versions/hashes; no redundant package managers |
| Tests | C++ domain tests, Qt Test/Quick Test for UI, image/project fixtures | Exact packages pinned in the initial scaffold |

See the [28-entry library register](../planning/LIBRARIES.md) for specific candidates, maturity evidence, evaluation and fallbacks.

## Graphics decision that must remain explicit

Qt Quick owns a [UI scene graph](https://doc.qt.io/qt-6/qtquick-visualcanvas-scenegraph.html). OPEN-TOON's compositor is a separate model, evaluated by the engine and presented as a viewport image/surface.

[QQuickRhiItem](https://doc.qt.io/qt-6/qquickrhiitem.html) integrates RHI rendering into Qt Quick. However, **QRhi has limited source/binary compatibility guarantees**, uses APIs associated with `Qt::GuiPrivate`, and does not automatically work through Qt Quick's software backend. Isolate it, pin Qt, and retain an independent CPU diagnostic path. Do not claim support for a Qt upgrade before rebuilding and verifying that adapter.

SP-02 compares reference CPU rasterization plus dirty-region uploads with a GPU RHI pipeline. Evaluate Skia as a vector-renderer implementation, not as another document model. The UI must not receive encoded PNG images or large JSON payloads for every pen movement.

## Versions and licenses

Pin stable versions **when implementation begins**, with lockfiles, compilers and a verified OS matrix. Documentation under `/latest/` is not a selected dependency version. [CMake presets](https://cmake.org/cmake/help/latest/manual/cmake-presets.7.html) share configurations, and [Conan](https://docs.conan.io/2/tutorial/consuming_packages/intro_to_versioning.html) documents package version management.

The repository initially adopts **GPL-3.0-or-later** for original contributions. This favors keeping redistributed project modifications open. A permissive license permits other integration models, including closed derivatives; any owner reconsideration should precede significant outside contributions. The [GPL v3 text](https://opensource.org/license/gpl-3.0) and [Qt licensing documentation](https://doc.qt.io/qt-6/licensing.html) are starting references. Qt licensing varies by module; do not assume one license applies uniformly.

FFmpeg requires examination of exact build flags, external libraries, notices and redistribution obligations; its [legal guide](https://ffmpeg.org/legal.html) distinguishes configurations. Running it as an external process does not automatically remove those obligations. Plugins and optional models need separate license review. This is dependency selection, not a blanket legal-compatibility guarantee.
