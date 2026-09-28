# HM-06 native control measurement

Measured on 2026-09-28 with the optimized `build/desktop` build on macOS
15.5 (24F74), Apple M1 Pro, 16 GiB RAM. The saved source project comes from
the revised 19-part, 1920×1080 Harmony Moment toon fixture. One upper-arm Part
has a keyed bone chain and the torso has a keyed curve. The native Qt Quick
window displayed a 1140×491 canvas at device pixel ratio 2.

The benchmark sends mouse events to the bone tip, waits for the window's
`frameSwapped` signal after each move, and checks that a live preview visibly
changed while the authored document remained untouched. It discards five warmup
moves, then reports 40 input-to-present samples per run. Process peak resident
memory comes from `getrusage(RUSAGE_SELF)` and includes the application and Qt.
The [captured preview](hm06-interaction-ui.png) shows the actual native window.

| Run | Median | p95 | Peak resident bytes |
|---|---:|---:|---:|
| 1 | 8.58 ms | 16.45 ms | 271,532,032 |
| 2 | 15.37 ms | 17.91 ms | 270,974,976 |
| 3 | 8.46 ms | 15.71 ms | 272,187,392 |
| 4 | 10.25 ms | 16.34 ms | 272,007,168 |

The measured subset is below the proposed 50 ms p95 and 2 GiB resident limits
on this host. `frameSwapped` measures application presentation, not display
scanout or stylus latency. This scene has 19 original parts but does not yet
include the complete audio, matte, camera and control workload. The full B4/HM
shot budget and the physical-device profile remain open.

Reproduce from the repository root with a foreground macOS display:

```sh
cmake --build build/desktop --target opentoon_render_tests open-toon --parallel 4
OPENTOON_HM06_BENCH_PROJECT="$PWD/build/hm06-interaction.otoon" \
  ctest --test-dir build/desktop -R 'Nineteen Harmony parts keep rest pixels' --output-on-failure
build/desktop/open-toon.app/Contents/MacOS/open-toon \
  --hm06-benchmark "$PWD/build/hm06-interaction.otoon"
```

The project and preview files under `build/` are generated test outputs. The
benchmark emits one JSON record per run; repeat the last command to observe
host variation.
