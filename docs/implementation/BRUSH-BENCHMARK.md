# Initial 4K raster benchmark

Measurement on 2026-09-20, Apple M1 Pro, 16 GiB RAM, macOS arm64, AppleClang 17,
Release build, pinned MyPaint 1.6.1 and Conan dependencies. This is one local run;
the machine was also compiling dependencies. It is not a release performance guarantee.

`opentoon_brush_benchmark NEW_PROJECT_PATH` draws 24 gestures with 480 samples each,
cycling ink, soft, dry and smudge presets on a 3840 × 2160 canvas. Every sample publishes
an immutable preview snapshot. It then saves eleven revisions with unchanged media
and changed scene names, reopens the last revision and compares the document.

| Measurement | Result |
|---|---:|
| Samples | 11,520 |
| Painting including snapshot publication | 1,592.85 ms |
| Median sample | 0.0955 ms |
| 95th percentile sample | 0.322417 ms |
| Maximum sample | 23.3771 ms |
| Allocated canvas tiles | 1,503 |
| Uncompressed final tile data | 46.97 MiB |
| Eleven saves | 1,700.68 ms |
| Load and semantic comparison | 217.724 ms |
| Project size including eleven revisions | 13,901,824 bytes |

This measures engine/snapshot work and storage, not Qt presentation, OS event delivery,
physical pen latency, a long drawing session, textured brush imports or many animated
4K drawings. The maximum sample and the eventual full UI latency require further
profiling before P05/P11 budgets can be closed. Save still recomputes resource hashes
and verifies reused data; unchanged resources occupy one compressed copy on disk.

Reproduce with a fresh output path. Do not compare timings from sanitizer builds or
heterogeneous CI machines with this Release run. The source fixture is procedural and
contains no third-party artwork.
