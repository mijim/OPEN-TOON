# macOS preview

An experimental.11 build provides a standalone **Apple Silicon (arm64)** application
for **macOS 15 or later**. It was tested on macOS 15.5. Intel Macs and other operating
systems are not qualified by this artifact.

## Download and opening the package

Download the [macOS arm64 ZIP](https://github.com/mijim/OPEN-TOON/releases/download/v0.2.0-experimental.11/OPEN-TOON-0.2.0-experimental.11-macOS-arm64.zip)
from the experimental.11 GitHub release.

If you build/package locally, extract the resulting ZIP and open `OPEN-TOON.app`.
Qt and its runtime libraries are bundled; users of that package do not need Homebrew,
Conan, Qt SDKs or command-line developer tools.

This preview is **ad-hoc signed**, without a Developer ID certificate or Apple
notarization. If macOS blocks the downloaded application, review the app-specific
**System Settings → Privacy & Security → Open Anyway** action. The release does not
require disabling Gatekeeper globally. An organization-managed Mac may restrict
opening unnotarized applications.

The local output also includes SHA-256 checksums, a runtime inventory and a separate
archive of dependency sources/build recipes. Those files are for verification and
rebuilding; only the application ZIP is needed to run OPEN-TOON.

## Build and package

First complete the locked build in [BUILD.md](BUILD.md). Deploy using the installed
Qt SDK, into an empty dedicated output directory:

```sh
python3 scripts/package_macos.py --version 0.2.0-experimental.11
```

The script audits every bundled Mach-O dependency, fixes split Homebrew framework
references, removes absolute SDK search paths and derives the minimum OS from the
binaries. It emits `runtime-manifest.json`; files ending `.local.json` are local
provenance and must not be published.

Before finalizing, supply `dependency-sources/sources.json` and the matching source
archives/recipes, plus `OPEN-TOON.app/Contents/Resources/ThirdParty/NOTICE.txt` and
component notices. For this exact local build these are recoverable from its dependency
source archive and application ZIP. [macos-dependencies.json](macos-dependencies.json)
records upstream URLs and verified archive hashes. If any dependency changes,
rebuild the inventory and corresponding source/notices rather than reusing old ones.

```sh
python3 scripts/finalize_macos.py --version 0.2.0-experimental.11
```

Finalization checks source hashes, signs nested code and the bundle ad hoc, verifies
the signature and produces the ZIP, source asset and checksums. Modified Qt libraries
can be substituted in the bundle and re-signed locally with the same process; no
private signing key or library-validation restriction is imposed by this preview.

## Verification boundaries

The local ZIP was extracted outside the workspace and its native smoke journeys
run with Qt/QML/DYLD SDK overrides removed. Dynamic-loader output was checked for
external Homebrew or developer-home libraries. This verifies the packaged runtime
on the development Mac; it is not a clean-machine, Intel, physical-tablet or complete
supported-OS qualification. See [status.json](status.json) for evidence and open gates.
