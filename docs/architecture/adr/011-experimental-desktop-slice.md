# ADR-011 — Experimental desktop implementation boundaries

Date: 2026-09-20. Status: accepted for the experimental implementation; production backend decisions remain open.

The owner authorized implementation through P11 and confirmed mouse input as the immediately available device. A physical tablet is unavailable for now. These facts authorize development; they do not waive the phase acceptance criteria.

## Decision

Implement one usable native editing path with a Qt-free C++20 document and application session, a SQLite storage adapter, a QtGui CPU rendering adapter, and a Qt Quick presentation layer. Preview, PNG export and the command-line fixture use the same scene evaluation. Only input/view adapters know about Qt events. The session validates a candidate document before publishing an immutable snapshot; failed commands publish nothing.

The experimental `.otoon` format stores complete JSON document revisions inside one SQLite database, including image bytes. DELETE journaling and FULL synchronization keep the complete revision inside one transaction. An expected head revision prevents a stale writer from saving over newer work. Existing files must have the application ID and supported database version before saving. Loading first attempts a non-creating read/write connection so SQLite can recover a hot rollback journal, then falls back to read-only access for clean read-only projects. It does not execute document-provided SQL.

Use QPainter/QImage as the initial reference renderer. This establishes behavior and repeatability; it does not select the final GPU backend. Pointer samples remain native in memory; no serialization occurs per sample. A committed edit currently copies the full document.

## Evidence and constraints

- Real process termination at four save boundaries, including a 4 MiB image that spills SQLite's cache, now has a regression test. The pinned SQLite build exposed the need for writable rollback recovery that the SDK version did not expose.
- Mouse events and synthetic pressure events pass through the actual QQuickWindow and canvas. No physical tablet, tilt mapping, eraser-end mapping or driver compatibility claim follows from synthetic tests.
- Forty-eight 1080p frames can be saved, reopened and rendered. This small fixture is not a large production benchmark.
- SQLite schema and document schema are independently checked. A future version is rejected; there is no migration engine yet.
- Complete revisions are limited to 64 MiB serialized JSON. Raw image bytes represented as JSON consume more space than raw RGBA.
- Undo uses full snapshots with an approximate 128 MiB / 100-step retention budget. The current scene and saved snapshot are additional memory. This is not a hard total-memory bound.
- Autosave runs synchronously and keeps full revisions. External immutable blobs, compaction, background saves, content-addressed resources and scalable incremental undo remain open.
- CPU rasterization, alpha and basic sRGB colors are available. OCIO, linear-light compositing, GPU caches and production color guarantees remain open.

## Alternatives and follow-up

The original SQLite-plus-immutable-blobs proposal remains the production direction in ADR-004. This temporary layout isolates storage evolution and enables failure testing without prematurely implementing a resource store. Do not claim the external-blob crash protocol has passed.

A bounded source audit of OpenToonz at `b43f9a83d8abb0b94a95c03da7664165a3fff6d6` found a Qt 5/C++17 application with tightly connected engine, application and tool libraries. No upstream code was copied. No successful upstream build or subsystem extraction has been established, so SP-00 remains open. See its [source build definition](https://github.com/opentoonz/opentoonz/blob/b43f9a83d8abb0b94a95c03da7664165a3fff6d6/toonz/sources/CMakeLists.txt).

Before closing P00/P01, finish the measured renderer/reuse comparisons and supported-platform input checks. Before P11, replace the bounded prototype limitations or explicitly amend scope with evidence and owner agreement. The P00–P11 target itself is unchanged.
