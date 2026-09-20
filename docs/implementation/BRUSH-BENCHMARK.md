# Initial 4K raster benchmark

Measurement on 2026-09-20, Apple M1 Pro, 16 GiB RAM, macOS arm64, AppleClang 17,
Release build, pinned MyPaint 1.6.1 and Conan dependencies. This is one local run;
no additional local build was dispatched during the final measurement, but OS load was not controlled. It is not a release performance guarantee.

`opentoon_brush_benchmark NEW_PROJECT_PATH` draws 24 gestures with 480 samples each,
cycling ink, soft, dry and smudge presets on a 3840 × 2160 canvas. Every sample publishes
an immutable preview snapshot. It then saves eleven revisions with unchanged media
and changed scene names, reopens the last revision and compares the document.

| Measurement | Result |
|---|---:|
| Samples | 11,520 |
| Painting including snapshot publication | 996.21 ms |
| Median sample | 0.082917 ms |
| 95th percentile sample | 0.145208 ms |
| Maximum sample | 0.431417 ms |
| Allocated canvas tiles | 1,497 |
| Uncompressed final tile data | 46.78 MiB |
| Eleven saves | 1,133.25 ms |
| Load and semantic comparison | 109.922 ms |
| Project size including eleven revisions | 13,840,384 bytes |

This measures engine/snapshot work and storage, not Qt presentation, OS event delivery,
physical pen latency, a long drawing session, textured brush imports or many animated
4K drawings. Tail latency under sustained load and the eventual full UI latency require further
profiling before P05/P11 budgets can be closed. Save still recomputes resource hashes
and verifies reused data; unchanged resources occupy one compressed copy on disk.

Reproduce with a fresh output path. Do not compare timings from sanitizer builds or
heterogeneous CI machines with this Release run. The source fixture is procedural and
contains no third-party artwork.
