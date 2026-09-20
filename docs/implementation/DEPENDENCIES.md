# Experimental dependency record

This records what the implementation actually uses. It is not a release SBOM. See `conanfile.py`, `conan.lock` and the CMake targets for the dependency boundary.

| Component | Verified local build | Role and license |
|---|---|---|
| Qt | 6.11.2; CI target 6.8.3 | Core, Gui, Qml, Quick, QuickControls2, QuickDialogs2 and their dependencies. Open-source Qt licensing is module/file-specific (LGPL/GPL and third-party notices). Dynamic desktop adapter; no Qt in domain/application. |
| SQLite | Conan 3.53.4, recipe `89fcf5cda598966acb7f3e185b19c58d` | Transactional revisions; public domain upstream. SDK 3.43.2 also tested locally. |
| nlohmann/json | 3.12.0, recipe `2d634ab0ec8d9f56353e5ccef6d6612c` | Versioned serialization behind storage adapter; MIT. |
| Catch2 | Conan 3.15.0, recipe `dad1ca2786e3f2c80129053a4ddc094f` | Behavioral tests only; BSL-1.0. System 3.16.0 also tested locally. |
| libmypaint | 1.6.1, commit `2768251dacce3939136c839aeca413f4aa4241d0` | Pressure-sensitive raster engine through a narrow C adapter; ISC, plus upstream third-party notices. No GLib/GEGL/OpenMP. |
| json-c | 0.18, Conan lock | MyPaint brush settings parser; MIT. |
| Zstandard | 1.5.7, Conan lock | Resource compression; BSD-3-Clause (dual-license upstream). |
| OpenSSL Crypto | 3.5.8, Conan lock | Resource SHA-256; Apache-2.0. No TLS/network functionality in the storage adapter. |
| CMake / Ninja / Conan | 4.4.3 / 1.13.2 / 2.20.1 locally | Build tooling, not application runtime. BSD-3-Clause / Apache-2.0 / MIT respectively; transitive tool notices remain separate. |

The Conan lock pins recipe revisions; it is not a byte-identical OS/toolchain/Qt lock. The local compiler is AppleClang 17, the SDK is macOS 15.5, and all builds in the initial evidence are arm64. No production latency or large-scene benchmark has qualified these choices yet.

Qt Test is discovered by the desktop build but is not a runtime dependency of the editor. The smoke workflow uses native events directly. qtmultimedia, Eigen and FFmpeg are available on the development machine but are **not linked into or used by this implementation**. Clipper2, Skia, OCIO, miniaudio, deformation libraries and other roadmap candidates remain unadopted.

## macOS preview redistribution preparation

Experimental.10 prepares a local macOS arm64 preview with 124 audited Mach-O files. The
application includes component license/copyright notices under
`Contents/Resources/ThirdParty`, an exact runtime dependency inventory and source
correspondence. A separate local archive contains verified upstream source archives,
Homebrew formulae and patches, Conan recipes and upstream SPDX inventories. See
[macos-dependencies.json](macos-dependencies.json) and [packaging notes](MACOS-PREVIEW.md).

Deployed Homebrew components include Qt base/declarative/quicktimeline/svg 6.11.2,
Brotli, D-Bus, double-conversion, FreeType, gettext, GLib, Graphite2, HarfBuzz, ICU,
libjpeg-turbo, libb2, libpng, md4c, PCRE2, OpenSSL 3.6.4 and Zstandard. Qt's deployed
plugins introduce dynamic GLib/OpenSSL dependencies even though the MyPaint adapter
omits GLib and storage uses static OpenSSL Crypto 3.5.8. Only the linked/deployed
versions in the release inventory describe this binary. Upstream SPDX inventories
can also describe upstream build dependencies; they are not the app's exact SBOM.

Binary publication was cancelled at the owner's request. The source tag is public;
the prepared archives have not been uploaded. The standalone ZIP passed native smoke outside the workspace without loading
Homebrew or developer-home libraries. Ad-hoc signatures verify. No third-party
fonts or artwork are bundled; original presets remain first-party code. Automated
release-wide SBOM generation, clean-machine qualification, Developer ID signing
and notarization remain open P11 work.
Primary sources: [Qt licensing](https://doc.qt.io/qt-6/licensing.html), [SQLite copyright](https://sqlite.org/copyright.html), [nlohmann/json license](https://github.com/nlohmann/json/blob/v3.12.0/LICENSE.MIT), [Catch2 license](https://github.com/catchorg/Catch2/blob/v3.15.0/LICENSE.txt). No proprietary animation source or artwork was copied.

The MyPaint archive hash and minimal upstream build are in `cmake/MyPaint.cmake`.
The source cache is build-only; configure downloads once and verifies SHA-256. Programmatic
presets are original code. The fallback is to retain raster tiles and disable unsupported
brush authoring rather than changing stored pixels. Current evidence is the replay and UI
workflow corpus; production brush latency remains an open gate. miniaudio is resolved in
Conan for the upcoming media adapter but is not currently linked or used.

The lock also includes Windows-only NASM and Strawberry Perl build requirements used by OpenSSL. They are build tools, not linked application libraries.
