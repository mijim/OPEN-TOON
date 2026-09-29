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

The render test checks all 480 frames at 480 × 270, five selected frames at
1920 × 1080 with one connected character silhouette in each, 32
bone-tip/follower positions across four parts and eight
frames, source substitution choices, saved state and exact pixels
after project reopen. The native Qt Quick smoke opens the project, visits six
beats, checks the three hand choices and torso curve, presents the canvas, then
saves and reopens an independent copy. The original 19-part intake fixture is
unchanged. The continuous 48-frame example now identifies its initial leg as
`leg_left` rather than the generic `Body` role.

Representative saved frames: [front speech](hm06-visual-shot-0120.png),
[pointing and bend](hm06-visual-shot-0240.png), and
[camera push](hm06-visual-shot-0360.png).

## Local macOS measurements

The optimized `build/locked` build ran on macOS 15.5, Apple M1 Pro with
16 GiB RAM. One complete 480-frame headless render at 1920 × 1080 averaged
5.72 ms/frame. This measures renderer throughput, not input latency.

The native Qt Quick benchmark used a 1140 × 491 canvas at device pixel ratio
2. Each run discarded five warmup moves, then measured 40 input-to-
`frameSwapped` drags of the left-arm tip without modifying the document.

| Sample frame | p95 in three runs | Peak resident bytes in three runs |
|---:|---:|---:|
| 12 | 17.50 / 18.10 / 17.51 ms | 283,000,832 / 283,852,800 / 285,179,904 |
| 360 | 17.41 / 17.28 / 17.03 ms | 283,541,504 / 284,655,616 / 285,212,672 |

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
