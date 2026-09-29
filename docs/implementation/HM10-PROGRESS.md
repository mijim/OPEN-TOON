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
positions; source-rate conversion uses bounded linear interpolation. Export
runs in the background, reports progress and atomically discards a cancelled
temporary file. **Play** uses a miniaudio output device when placed clips exist.
The callback mixes the immutable scene snapshot into bounded stack buffers;
submitted sample position drives the playhead. Seeking during playback resets
the sample cursor. The scene loops at its exact rational sample boundary.
Dragging across the timeline ruler or drawing rows previews 80 ms audio
fragments from each marked frame and stops on release. Moving a waveform
continues to edit clip placement instead.
Editing or device interruption stops playback; a failed device open leaves the
visual preview available and reports the failure.

The saved document supports up to 64 audio assets, 1,000 clips, 512 MiB total
audio and 128 MiB per WAV asset. Formats 1–15 load without audio; format 16
clips load with one repeat. First saves of an older schema create a readable
source-version backup. See [ADR-036](../architecture/adr/036-pcm-audio-document.md)
for interval and frame-edit semantics and [ADR-038](../architecture/adr/038-repeated-audio-clips.md)
for sample-contiguous repeats.

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
- A 600-second fractional-rate, two-source 1024-frame callback workload
  measured p95 **0.014 ms** against a 21.33 ms output period on the local
  M1 Pro macOS Release build. It checks the exact scene sample count and a
  rendered cue; this is a mixer cost sample, not a hardware underrun trace.
- Local macOS `build/locked`: 154/154 CTest entries pass. The native
  `--hm10-smoke` loaded the Qt Quick audio timeline, checked its cue sample,
  captured and visually inspected `build/hm10-audio-smoke.png`, exported an
  exactly sized WAV with its cue at the correct sample, advanced and sought
  its native playhead through the null backend, dragged a waveform four frames
  and undid it, scrubbed the ruler through frame two, set two repeats and
  verified the second cue in both waveform and WAV output, then undid import.
  The pre-existing HM-07 native dashboard smoke still passes.

## Open contract

HM-10 remains incomplete. Device latency calibration, device-loss recovery,
hardware underrun and dropped-frame traces, production-quality rate
conversion, hardware audible scrub quality and a full
ten-minute audiovisual drift run are pending. The miniaudio adapter is adopted
for this experimental desktop profile; hardware and cross-platform
qualification remain open.
