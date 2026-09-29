# HM-06 native control measurement

Measured on 2026-09-28 with the optimized `build/desktop` build on macOS
15.5 (24F74), Apple M1 Pro, 16 GiB RAM. The saved source project comes from
the latest original 19-part, 1920×1080 Harmony Moment toon fixture. One upper-arm Part
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
| 1 | 8.52 ms | 16.76 ms | 272,203,776 |
| 2 | 8.32 ms | 14.90 ms | 271,073,280 |
| 3 | 8.38 ms | 16.05 ms | 270,336,000 |
| 4 | 8.39 ms | 15.96 ms | 272,203,776 |

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

## Continuous-limb candidate, 2026-09-29

The same native drag benchmark was run once on the 15-artwork-Part candidate,
where both arms and both legs are full-length single-image meshes. On the same
Apple M1 Pro and macOS 15.5 host, 40 samples gave median 16.44 ms, p95 17.26 ms,
and peak resident memory 274,071,552 bytes. The measured canvas was 1140 × 491
logical pixels at device pixel ratio 2. This is a separate workload from the
19-part measurements above and remains a subset of the complete shot. This
first measurement predates the saved bone-tip links.

After format 10 added four parent bone-tip links for the hands and feet, one
40-sample run on the same host measured median 16.38 ms, p95 17.18 ms and peak
resident memory 275,398,656 bytes. The linked 15-part subset meets the proposed
50 ms/2 GiB limits on this host. A complete shot and repeated-run variance
remain unmeasured.

After correcting the link anchor for source substitutions with a different
rest tip/direction, three 40-sample runs on the saved 48-frame example measured
p95 17.86, 18.15 and 17.92 ms. Peak resident memory was 278,609,920,
278,315,008 and 278,347,776 bytes. The extra frame-zero binding lookup and
direction calculation stayed within the proposed limits on this host. The
earlier single run is retained as historical evidence, not a controlled
before/after speed comparison.

With the anchor saved explicitly in format 11, three more 40-sample runs on
the current example measured p95 18.00, 18.22 and 18.45 ms. Peak resident
memory was 277,200,896, 278,052,864 and 279,216,128 bytes. All remain below
the proposed subset limits. These runs are not a controlled comparison with
the preceding format-10 measurements; host load and frame scheduling vary.

After the joined trouser-waist redraw, the regenerated 15-Part example used
one 512 × 512 waist image in place of the 256 × 256 pelvis image. Three native
40-sample drags on the same host measured p95 31.11, 17.24 and 16.94 ms;
peak resident memory was 277,331,968, 278,593,536 and 276,430,848 bytes.
All three meet the proposed subset limits. The first run had a higher frame
tail, so this is qualification evidence for the changed art, not proof of a
render-speed improvement. The complete B4/HM scene is still open.

After rounding both sleeve silhouettes, three more 40-sample drags on the
regenerated project measured p95 17.97, 16.69 and 16.98 ms. Peak resident
memory was 277,725,184, 277,430,272 and 279,445,504 bytes. These runs again
meet the subset limits; the full shot and physical tablet remain unmeasured.

Reproduce after building the render tests and desktop app:

```sh
OPENTOON_HM06_CONTINUOUS_PROJECT="$PWD/build/hm06-continuous.otoon" \
  build/desktop/opentoon_render_tests \
  'Continuous Harmony limbs bend as four single meshes and reopen identically'
build/desktop/open-toon.app/Contents/MacOS/open-toon \
  --hm06-benchmark "$PWD/build/hm06-continuous.otoon"
```

## Twenty-second deformation workload, 2026-09-29

The editable 48-frame continuous rig was retimed through the application's
range operation to 480 frames at 24 fps. The test checks all 480 frames at
480 × 270 preview size, selected full-resolution frames against the original
timing, saved/reopened equivalence and preservation of the four bone-tip
links. One optional 480-frame 1920 × 1080 renderer run averaged 5.07 ms/frame
on this host. That number measures headless render throughput, not input
latency.

The native Qt Quick drag benchmark opened the saved 480-frame project at a
1140 × 491 logical-pixel canvas, device pixel ratio 2. It used five warmup
drags followed by 40 input-to-`frameSwapped` samples per run:

| Run | Median | p95 | Peak resident bytes |
|---|---:|---:|---:|
| 1 | 16.59 ms | 17.61 ms | 277,692,416 |
| 2 | 16.57 ms | 18.28 ms | 276,643,840 |
| 3 | 16.65 ms | 17.12 ms | 276,987,904 |

This exercises the complete scene duration and the same bound 15-part rig,
but its original 48-frame pose sequence is stretched. It has no audio,
animated camera, mattes or published animator controls. The measurements
therefore qualify this deformation workload only; they do not close the full
B4/HM shot budget or artistic acceptance.

A later [480-frame visual shot](HM06-VISUAL-SHOT.md) adds timed face, mouth and
hand changes, a torso curve, character travel and an output-camera push. Its
native input measurements sample both an early front pose and the later
camera/alternate-sleeve pose. Audio, mattes and published controls remain open.

Reproduce from the repository root with the optimized macOS build:

```sh
OPENTOON_HM06_LONG_PROJECT="$PWD/build/desktop/hm06-long-study.otoon" \
  build/desktop/opentoon_render_tests \
  'Full-length continuous toon retimes linked limbs and coordinated views'
OPENTOON_HM06_FULL_RENDER=1 build/desktop/opentoon_render_tests \
  'Full-length continuous toon retimes linked limbs and coordinated views'
build/desktop/open-toon.app/Contents/MacOS/open-toon \
  --hm06-benchmark "$PWD/build/desktop/hm06-long-study.otoon"
```
