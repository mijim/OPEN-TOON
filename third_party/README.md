# Third-party notices

This directory records notices for the new raster/storage dependency boundary.
Versions and exact source pins are in `conan.lock`, `cmake/MyPaint.cmake` and
`docs/implementation/DEPENDENCIES.md`. libmypaint sources are fetched unchanged;
its fastapprox headers carry BSD-3-Clause notices and its Knuth random generator
is public domain. No third-party brushes or artwork are included.

These files are not yet a complete binary-distribution inventory: Qt and its
platform plugins, existing dependencies, fonts and system libraries must be
inventoried against the actual deployable package before a public binary release.
