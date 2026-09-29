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
the source. **Scene → Export PCM WAV mix** writes a 48 kHz stereo PCM16 mix
from an immutable document snapshot. Multiple clips sum at exact scene sample
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

## Verification

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
- Local macOS `build/locked`: 164/164 CTest entries pass. The native
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
