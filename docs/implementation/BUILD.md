# Build and verify the experimental editor

The application is C++20 with Qt 6.8 or newer. Documentation tools use Python 3.11+. The verified local environment is macOS 15.5 arm64, AppleClang 17 and Qt 6.11.2. Qt 6.8.3 builds and automated tests also pass on Windows Server 2022, macOS 14 and Ubuntu 24.04 in [CI](https://github.com/mijim/OPEN-TOON/actions/runs/35531337766). This is build/test coverage, not full device or installer qualification.

## Locked core dependencies

Install CMake 3.25+, Ninja, Python, a C++20 compiler and the open-source Qt SDK (Core, Gui, Qml, Quick, QuickControls2, QuickDialogs2 and Test). Add the SDK prefix to `CMAKE_PREFIX_PATH`. Qt is supplied separately; `conan.lock` pins the storage/JSON/test recipes, not the Qt SDK or the entire toolchain.

```sh
python3 -m venv build/tooling
build/tooling/bin/pip install conan==2.20.1
build/tooling/bin/conan profile detect
build/tooling/bin/conan install . --lockfile=conan.lock \
  --output-folder=build/dependencies --build=missing \
  -s compiler.cppstd=20 -s build_type=Release
cmake -S . -B build/locked -G Ninja -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_TOOLCHAIN_FILE="$PWD/build/dependencies/build/Release/generators/conan_toolchain.cmake"
cmake --build build/locked --parallel
ctest --test-dir build/locked --output-on-failure
```

On Windows, use `build/tooling/Scripts/python.exe -m pip` and `build/tooling/Scripts/conan.exe` instead of the POSIX virtual-environment paths. Pass an absolute toolchain path appropriate to the shell. With MSVC, Conan uses a multi-configuration layout: the toolchain is under `build/dependencies/build/generators/`, without `Release/`. Configure with `-G "Visual Studio 17 2022"`, then build with `--config Release` and test with `-C Release`. The CI workflow contains the Windows invocation.

To build without Qt, add `-DOPENTOON_DESKTOP=OFF`. The document, commands, serialization and storage still compile and test independently. To enable address/undefined-behavior sanitizers on Clang/GCC, add `-DOPENTOON_SANITIZERS=ON`.

## Convenient system-package build on macOS

```sh
brew install cmake ninja qtbase qtdeclarative qtsvg nlohmann-json catch2 json-c zstd openssl@3
cmake --preset desktop -DCMAKE_PREFIX_PATH=/opt/homebrew
cmake --build --preset desktop --parallel
ctest --preset desktop
open build/desktop/open-toon.app
```

Use the actual Homebrew prefix on Intel machines. System packages may differ from the lockfile. The first local system build used SDK SQLite 3.43.2 and Catch2 3.16.0; the locked build uses SQLite 3.53.4 and Catch2 3.15.0. Always report which build was tested.

## Run and inspect

```sh
# macOS; Linux/Windows use the open-toon executable in the build directory.
open build/locked/open-toon.app
build/locked/open-toon.app/Contents/MacOS/open-toon --smoke-test
QT_QPA_PLATFORM=offscreen build/locked/open-toon.app/Contents/MacOS/open-toon \
  --render-demo build/example-export
build/locked/open-toon.app/Contents/MacOS/open-toon \
  --inspect build/example-export/bouncing-ball.otoon
```

`--smoke-test` creates a separate test application profile, sends mouse and synthetic tablet events through the native window, checks undo/redo and save/reopen, and writes `build/ui-smoke.png`. Run from the repository root with an available display. `--render-demo` writes a synthetic scene and 48 PNG frames; use a fresh output directory. It is a verification command, not a general render-farm interface.

## Packaging

CMake contains Qt deployment rules. A local development bundle is not a signed/notarized release or evidence of clean-machine compatibility. Split Homebrew Qt formulae can require explicit library/import paths for `macdeployqt`; validate the resulting bundle independently. CI currently builds/tests source and does not publish installers.

See [status](STATUS.md), [user guide](USER-GUIDE.md), [dependency record](DEPENDENCIES.md) and [ADR-011](../architecture/adr/011-experimental-desktop-slice.md) before making support or performance claims.

## Raster benchmark

The locked build also produces `opentoon_brush_benchmark`. Pass a new project path;
existing files are refused. The command measures 11,520 samples on a 4K canvas,
11 saves sharing media and verified reopening. It emits JSON timing data. See
[the measurement report](BRUSH-BENCHMARK.md) for the current result and limitations.

libmypaint is fetched during configuration from a source commit and SHA-256 pinned
in `cmake/MyPaint.cmake`; the build requires Python and a C11 compiler as well as C++20.
[CI run 35531337766](https://github.com/mijim/OPEN-TOON/actions/runs/35531337766) verifies these additions on Windows, macOS and Linux; device and installer qualification remain separate.
