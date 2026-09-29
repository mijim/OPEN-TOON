# HM-06 twenty-second visual shot study

The [openable project](../../examples/clockwork-visual-shot.otoon) covers 480
frames at 1920 × 1080 and 24 fps. It uses the original, redistributable
Clockwork Hello art and the continuous four-limb rig. The construction test
reads the agreed `shot.json` cue frames and compares the saved project with the
scene it built.

The character has 15 artwork Parts, four bone-tip links, two bindings for the
left arm's sleeve substitutions, three other limb bones and one torso curve.
Front and three-quarter views switch at frames 120 and 432. Eight side-view
mouth drawings follow nine timing markers; open, pointing and fist drawings
change on the linked left hand. The character moves across the frame, and the
orthographic camera pushes in from frames 336–360 before returning to rest.
Frame 479 renders identically to frame zero.

The render test checks all 480 frames at 480 × 270 with one connected
character silhouette in every preview, plus seven selected frames at
1920 × 1080 with the same connectivity. It checks 1,920 bone-tip/follower
positions across four parts and every frame, source substitution choices,
saved state and exact pixels
after project reopen. The native Qt Quick smoke opens the project, visits six
beats, checks the three hand choices and torso curve, presents the canvas, then
saves and reopens an independent copy. It also opens an intentionally unmatched
sleeve, matches its incoming pose through the native controller, undoes, redoes,
and reopens the correction. The original 19-part intake fixture is unchanged.
The continuous 48-frame example identifies its initial leg as `leg_left`
rather than the generic `Body` role.

A full-frame continuity sweep caught a 5.47 px left-wrist step at frame 300:
the incoming sleeve and outgoing sleeve had different evaluated angle curves.
The visual shot uses **Match previous pose** to key the incoming bone to the
outgoing evaluated pose at that substitution boundary. Every linked tip
advances under 4 px per frame; the left wrist advances under 2 px at frames
300 and 432. Matching is an explicit authoring command for compatible rest
controls, not an automatic cross-substitution evaluation rule.

Representative saved frames: [front speech](hm06-visual-shot-0120.png),
[pointing and bend](hm06-visual-shot-0240.png), and
[camera push](hm06-visual-shot-0360.png).
The [frame before the sleeve change](hm06-visual-shot-0299.png) and
[the incoming sleeve frame](hm06-visual-shot-0300.png) are retained at
1920 × 1080 for direct contour inspection. Their left shoulder, elbow,
cuff and hand remain visually joined across the change.
The later cubic trouser-contour correction regenerated this project and the
five displayed stills without changing the 480-frame timing, bone joints or
substitution decisions. All frames retain one connected silhouette and the
saved example matches the constructed document in the 166-entry local suite.
The current saved example rebinds the four one-piece limbs and both left-sleeve
substitutions to alpha-following 6 × 16 contour meshes with a 40 px elbow/knee
transition. The five displayed stills and integrated audio study were
regenerated again with sampled artwork edges and the original two-view cap.
All 480 preview frames and seven full-resolution frames retain
one connected silhouette, and the current 179-entry suite plus native HM-07
and integrated-shot smokes pass. A full-resolution 480-frame sweep measured
5.00 ms/frame on this host. One current 40-sample native run measured p95
17.84 ms at frame 12 and 18.22 ms at frame 360, with peak resident memory
291.1 MB and 290.5 MB respectively. These measurements cover the visual
shot; complete audio/matte/control interaction budgets remain open.

## Local macOS measurements

The optimized `build/locked` build ran on macOS 15.5, Apple M1 Pro with
16 GiB RAM. One complete 480-frame headless render at 1920 × 1080 averaged
5.45 ms/frame with the earlier rectangular limb mesh and matched sleeve key.
This measures renderer throughput,
not input latency.

The native Qt Quick benchmark used a 1140 × 491 canvas at device pixel ratio
2. Each run discarded five warmup moves, then measured 40 input-to-
`frameSwapped` drags of the left-arm tip without modifying the document.
The harness reacquires the native window after macOS focus loss and retries
an interrupted move outside the measured sample.

| Sample frame | p95 in three runs | Peak resident bytes in three runs |
|---:|---:|---:|
| 12 | 18.50 / 19.10 / 16.93 ms | 283,656,192 / 284,901,376 / 281,919,488 |
| 360 | 21.75 / 17.30 / 17.42 ms | 285,605,888 / 282,722,304 / 283,639,808 |

Both sampled poses meet the proposed 50 ms p95 and 2 GiB resident limits on
this host. Frame 360 includes the alternate sleeve, three-quarter face,
side mouth, torso curve and camera push. The benchmark is still a bounded
visual scene; it has no audio clips, waveform/playback clock, matte nodes or
published animator controls. Those belong to HM-10, HM-12 and HM-07 before
full HM-15 qualification. Physical tablet input and owner artistic acceptance
also remain open.

Reproduce from the repository root:

```sh
build/locked/opentoon_render_tests \
  'Twenty-second visual shot combines deformers views mouth hands and camera'
OPENTOON_HM06_VISUAL_FULL_RENDER=1 build/locked/opentoon_render_tests \
  'Twenty-second visual shot combines deformers views mouth hands and camera'
build/locked/open-toon.app/Contents/MacOS/open-toon --smoke-test
build/locked/open-toon.app/Contents/MacOS/open-toon \
  --hm06-benchmark "$PWD/examples/clockwork-visual-shot.otoon"
OPENTOON_HM06_BENCH_FRAME=360 build/locked/open-toon.app/Contents/MacOS/open-toon \
  --hm06-benchmark "$PWD/examples/clockwork-visual-shot.otoon"
```

Set `OPENTOON_HM06_VISUAL_PROJECT` to a fresh path while running the first
render test to regenerate a project for inspection. The committed example is
compared structurally and pixel-wise with the constructed scene.
