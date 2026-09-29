# HM-10 audio progress

HM-10 is in progress. Format 16 introduced import of a local mono or
stereo PCM16 WAV file at the current scene frame. The original bytes and source
sample metadata are embedded as an immutable project resource. Unsupported or
corrupt WAV files report an error and leave the document unchanged.

The timeline draws a separate waveform row for each clip. A derived 256-sample
peak tree answers exact source-sample intervals, including partial edge bins,
from the scene's rational frame rate. It reuses immutable media while panning
or changing frame width, and rebuilds when a scene reuses an asset ID. Dragging
a waveform previews a new frame position and commits one undoable move on
release; Escape cancels. Format 17 adds 1–64 nondestructive repeats to each
clip. The Audio panel also moves a clip by frame, edits its half-open
source-sample in/out range, changes linear gain and repeat count,
and removes the clip. Every change is undoable; trim and gain do not rewrite
the source. Middle edge handles on a 48 kHz single-pass waveform now trim at
scene frames. The left edge advances scene start and source in-sample together;
the right edge changes the source out-sample. A drag previews its edge, Escape
cancels, and release commits one undoable edit. Numeric sample trim remains
available for other rates and repeated clips. See
[ADR-052](../architecture/adr/052-frame-aligned-audio-edge-trim.md).
**Duplicate** creates a second independently editable clip at
the playhead with the same
source reference, trim, gain, repeats and fades; it keeps one embedded WAV
resource and commits one undo step. Format 22 adds linear fade-in/out durations
in original source samples across the complete repeated clip. The two values
are set atomically; shorter trims or repeat counts clamp them to the new
length. They shape
preview and export through the same mix plan without changing source bytes.
The waveform retains source peaks and gain, with visible upper/lower fade
guides over the repeated clip; these guides indicate the envelope endpoints
without replacing the exact source-peak trace. Drag either small upper guide
handle to preview a new fade length in source samples; release commits one
undoable edit and Escape cancels.
Format 23 adds an undoable **Mute** control to each clip. It excludes the clip
from playback, scrub and both WAV exports without modifying its source or
other edits. The timeline dims and labels its retained source waveform;
**Unmute** restores its previous mix contribution. See
[ADR-047](../architecture/adr/047-per-clip-audio-mute.md).
Format 26 adds an undoable per-clip **Solo** control. If any clip is soloed,
only unmuted soloed clips enter preview, scrub and WAV export; mute takes
precedence. Excluded waveforms stay visible and dim. Duplicate and split copy
the solo flag without duplicating source bytes. Older clips load unsoloed.
See [ADR-051](../architecture/adr/051-persistent-audio-clip-solo.md).
Format 27 adds per-clip stereo **Balance** from -1 (left) through 0 (center)
to +1 (right). It attenuates the opposite output channel after the clip's
gain and envelope; a mono source uses the same value on its two output
channels, and a stereo source keeps its original channel content. The saved
number affects preview, scrub and WAV export and is undoable. Older projects
load centered. See [ADR-053](../architecture/adr/053-audio-clip-balance.md).
**Split** divides a 48 kHz single-pass clip at the playhead without copying
its WAV. The two placements meet at the exact source sample implied by the
scene's rational frame rate; gain and mute survive, and the outer fades stay
on their respective sides. The cut must leave source samples on both sides
and cannot cross a fade. Other source rates and repeated clips report an
error without editing. This subset preserves the canonical 48 kHz WAV mix;
output-device resampling near a cut remains unqualified. See
[ADR-050](../architecture/adr/050-frame-aligned-audio-split.md).

**Scene → Export PCM WAV mix** writes a 48 kHz stereo PCM16 mix from an
immutable document snapshot. Multiple clips sum at exact scene sample
positions. Export runs in the background, reports progress and atomically
discards a cancelled temporary file. Conversion between different sample
rates uses a precomputed 32-tap band-limited filter, including upsampling;
equal-rate sampling preserves source samples. **Scene → Export selected PCM WAV range** writes the selected
half-open frame interval at exact rational 48 kHz sample boundaries. Its PCM
payload equals the corresponding full-mix slice; invalid ranges and cancelled
jobs do not replace an existing output.
**Play** uses a miniaudio output device when placed clips exist.
The callback mixes the immutable scene snapshot into bounded stack buffers;
submitted sample position drives the playhead. Seeking during playback resets
the sample cursor. The scene loops at its exact rational sample boundary.
**Loop playback** is on by default. Turning it off makes the device emit
silence after the scene's exact end sample and stops the UI on the last frame;
silent preview follows the same rule. The toggle is view state and leaves the
document and original WAV untouched. See
[ADR-064](../architecture/adr/064-playback-loop-boundary.md).
**Range** plays the selected half-open timeline frame interval. Starting
outside it jumps to its first frame; Loop wraps at the rational end sample,
and Play once leaves its last frame visible. Seeking outside an active range
stops playback. This view-only mode clears when another scene opens. The
callback and silent preview share the interval; see
[ADR-065](../architecture/adr/065-selected-range-playback.md).
Dragging across the timeline ruler or drawing rows previews 80 ms audio
fragments from each marked frame and stops on release. Moving a waveform
continues to edit clip placement instead.
Editing or device interruption stops playback; a failed device open leaves the
visual preview available and reports the failure.
Playback diagnostics count mixer callbacks over their own output period and
playhead frames skipped by the UI timer. These are scheduling proxies; actual
device underruns and presented-frame drops need platform instrumentation.

The saved document supports up to 64 audio assets, 1,000 clips, 512 MiB total
audio and 128 MiB per WAV asset. Formats 1–15 load without audio; format 16
clips load with one repeat. First saves of an older schema create a readable
source-version backup. See [ADR-036](../architecture/adr/036-pcm-audio-document.md)
for interval and frame-edit semantics and [ADR-038](../architecture/adr/038-repeated-audio-clips.md)
for sample-contiguous repeats.
See [ADR-041](../architecture/adr/041-audio-downsampling-kernel.md) for the
shared bounded rate-conversion kernel and callback memory boundary.
See [ADR-042](../architecture/adr/042-selected-audio-export-range.md) for
selected frame-range export boundaries.
See [ADR-046](../architecture/adr/046-sample-accurate-audio-fades.md) for
source-sample fades and trim/repeat clamping.

## Verification

- Exact PCM checks cover centered, halfway-right and full-left mono cues,
  independent copied-clip balance, invalid-input rollback, undo/redo and
  save/reopen without changing the original WAV. A format-26 migration test
  loads centered, keeps a readable backup and rejects a missing current
  balance field. Native Qt Quick smoke edits the Balance field, checks the
  output channel and undoes it.

- At 24 and 24000/1001 fps, left/right frame-edge trims preserve every
  surviving canonical 48 kHz PCM sample. Tests check source coordinates,
  one-step undo/redo, rejected out-of-WAV edits and save/reopen. Native Qt
  Quick smoke drags both middle edge handles, checks Escape cancellation and
  verifies the exact samples; the trimmed-row screenshot was inspected.

- Two independent cue clips verify one and multiple solo selections, mute
  precedence, exact mix samples, undo/redo, invalid-command rollback and
  save/reopen. A format-25 fixture defaults to unsoloed, keeps a readable
  backup and rejects a malformed format-26 solo field. Native Qt Quick smoke
  clicks Solo and undoes it.

- A 48 kHz tone with gain and outer fades splits at frame 12 at both 24 and
  24000/1001 fps. The complete before/after stereo PCM output is byte-identical;
  the WAV resource, undo/redo and reopened project agree. Boundary, repeat,
  fade-crossing and 44.1 kHz attempts reject. Native Qt Quick smoke clicks
  Split, checks the mix and undoes one step.

- Two shared-source clips prove that muting one removes only its exact PCM
  contribution, muting both yields silence, and undo/redo and save/reopen
  preserve original bytes. A format-22 migration test defaults to unmuted,
  rejects a malformed current-schema clip and retains `.pre-v22.bak` on first
  save. Native HM-10 smoke clicks Mute and undoes it in one step; the inspected
  `build/hm10-muted-smoke.png` shows the dim `Muted` row and compact Unmute
  control.
- A domain and storage test duplicates a trimmed, repeated, faded clip at
  frame 24, confirms two independently identified placements share one WAV,
  checks four exact mixed cues, atomic undo/redo, invalid-copy rollback and
  save/reopen. Native HM-10 smoke clicks **Duplicate** in Qt Quick,
  checks the second placement and source count, then undoes once.
- `tests/audio_tests.cpp` checks malformed import rejection, exact retained
  bytes, a cue at sample 2002, atomic undo/redo, nondestructive clip edits,
  format-17 save/reopen, rational 24/24000/1001 positions, sample-contiguous
  trimmed repeat cues and overlapping
  44.1/48 kHz sources in one stereo mix. Exact peak-index queries match the
  direct scan across 300 intervals and a right-channel-only PCM cue. A 30 s
  source built its index in 3.52 ms; 1,000 varied queries took 0.52 ms versus
  1.47 ms for direct scans on the local M1 Pro Release profile.
- `tests/storage_tests.cpp` loads format-15 and format-16 scenes and verifies
  `.pre-v15.bak` and `.pre-v16.bak` after their first current-format saves.
- `tests/export_tests.cpp` imports an actual WAV through `EditorController`,
  samples the visible cue, edits, saves/reopens and rejects a missing file.
  It checks exact 24/fractional WAV lengths, cancellation preservation and
  byte-identical output before/after project reopen. A null device test starts,
  seeks and stops playback from an immutable scene without reaching speakers.
  A controller test verifies that Play follows this cursor, a manual seek
  updates it, a bounded scrub follows frame changes, and an edit stops preview
  before changing the document. The
  local host output device also opened successfully without starting playback.
  A new-scene test verifies that a reused asset ID cannot expose old peaks.
  A rational-end test checks trimmed clips at 24 and 24000/1001 fps. A
  controller test saves, reopens and exports a two-repeat cue byte-identically.
  The null device test checks callback diagnostics and the controller retains
  those counts after stopping preview.
- Null-device and editor transport tests now cover one-pass completion at the
  exact rational end sample, loop wrap, final-frame stop and unchanged scene.
  A silent transport test checks the same last-frame stop and wrap. Native HM-10 smoke clicks the loop toggle and checks play-once completion.
- A selected-range test checks exact integer and 24000/1001 device end samples,
  loop wrap, invalid
  bounds, silent preview, outside-range seek and unchanged document. Native
  HM-10 smoke clicks Range and runs an imported WAV from the first selected
  frame to the last included frame.
- A 600-second fractional-rate, two-source 1024-frame callback workload
  measured p95 **0.014 ms** against a 21.33 ms output period on the local
  M1 Pro macOS Release build. It checks the exact scene sample count and a
  rendered cue; this is a mixer cost sample, not a hardware underrun trace.
- A 96-to-48 kHz tone check suppressed 30 kHz input to 0.000089 output RMS
  while retaining 1 kHz at 0.561 RMS. A repeated 96 kHz cue remained aligned
  across the loop, and split versus whole output blocks were byte-identical.
  Two simultaneous 96 kHz tracks measured 0.232 ms p95 for a 1,024-frame
  block against a 21.33 ms callback period on the local M1 Pro Release build.
- The same filter now reconstructs an 8-to-48 kHz source tone at 3 kHz (0.560 RMS);
  whole and split output blocks agree. Equal-rate cue sampling remains direct.
- A constant PCM fixture checks fade endpoint samples, the full-volume
  repeated-cycle seam, invalid sum rejection, atomic undo/redo, trim clamping
  and source-byte preservation. A format-21 project loads with zero fades,
  saves chosen values and retains a readable `.pre-v21.bak`. Native HM-10
  smoke finds both controls, exports a faded mix and verifies an attenuated
  first cue with the second cue unchanged. The native screenshot
  `build/hm10-fade-smoke.png` was inspected: both endpoint guides appear on
  the repeated clip, and smoke compares the timeline row before/after the edit.
  Native mouse gestures drag both upper handles by one frame, then check the
  exact 2,000-sample change, one-step undo/redo and Escape cancellation.
- A range-export integration test compares selected PCM payload with the
  full-scene mix slice at 24 and 24000/1001 fps, checks exact length and
  rejects invalid boundaries before writing. The native HM-10 smoke exports
  and compares a two-frame range containing the known cue.
- `opentoon_audio_sync_benchmark` drives a silent repeated clip through the
  same adapter and samples its cursor every 8 ms. A two-second CoreAudio probe
  at 24 fps recorded 328 callbacks, zero callbacks over period, 0.043 ms
  maximum callback time and 142 final drift samples relative to the host
  monotonic clock after a one-second baseline. Two ten-minute silent CoreAudio
  runs on this M1 Pro measured, respectively, **24 fps:** 65,418 callbacks,
  zero processing overruns, 0.209 ms maximum callback time, 569 final drift
  samples, 778 maximum jitter samples and one skipped polled playhead frame;
  **24000/1001 fps:** 65,417 callbacks, zero processing overruns, 0.238 ms
  maximum callback time, 262 final drift samples, 565 maximum jitter samples
  and two skipped polled frames. Each drift is relative to the host monotonic
  clock after the first one-second baseline. These silent probes do not
  measure speaker delivery time, audible sync or hardware underruns.
- `tests/integrated_shot_tests.cpp` opens the original 480-frame continuous
  toon, binds the eye Part to a visible head cutter, embeds nine original
  synthetic cue tones at the mouth-change frames and saves the assembled
  `examples/clockwork-integrated-study.otoon`. It checks exact cue samples,
  mouth drawings, preserved source WAV bytes, graph validity, five rendered
  frames, exact 960,000-sample stereo WAV export and semantic save/reopen.
  Native `--open examples/clockwork-integrated-study.otoon
  --hm-integrated-smoke` checks the opened project, timeline and visual frame
  change in a Qt Quick window; its screenshot was inspected. These tones are
  timing markers, not spoken dialogue or a lip-sync quality test.
- Local macOS `build/locked`: 184/184 CTest entries pass. The native
  `--hm10-smoke` loaded the Qt Quick audio timeline, checked its cue sample,
  captured and visually inspected `build/hm10-audio-smoke.png`, exported an
  exactly sized WAV with its cue at the correct sample, advanced and sought
  its native playhead through the null backend, dragged a waveform four frames
  and undid it, scrubbed the ruler through frame two, set two repeats and
  verified the second cue in both waveform and WAV output, then undid import.
  The pre-existing HM-07 native dashboard smoke still passes.

## Open contract

HM-10 remains incomplete. Device latency calibration, device-loss recovery,
hardware underrun and presented-frame traces, complete rate-ratio and
broader rate-ratio quality qualification, hardware audible scrub quality and a full
ten-minute audiovisual drift run with visual output are pending. The miniaudio adapter is adopted
for this experimental desktop profile; hardware and cross-platform
qualification remain open.
