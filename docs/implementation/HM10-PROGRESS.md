# HM-10 audio progress

HM-10 is in progress. The first format-16 subset imports a local mono or
stereo PCM16 WAV file at the current scene frame. The original bytes and source
sample metadata are embedded as an immutable project resource. Unsupported or
corrupt WAV files report an error and leave the document unchanged.

The timeline draws a separate waveform row for each clip. Peaks use exact
source-sample intervals derived from the scene's rational frame rate, and
remain aligned when timeline frame width changes. The Audio panel moves a clip
by frame, edits its half-open source-sample in/out range, changes linear gain,
and removes the clip. Every change is undoable; trim and gain do not rewrite
the source. **Scene → Export PCM WAV mix** writes a 48 kHz stereo PCM16 mix
from an immutable document snapshot. Multiple clips sum at exact scene sample
positions; source-rate conversion uses bounded linear interpolation. Export
runs in the background, reports progress and atomically discards a cancelled
temporary file. Audio does not yet play inside the app.

The saved document supports up to 64 audio assets, 1,000 clips, 512 MiB total
audio and 128 MiB per WAV asset. Formats 1–15 load without audio. The first format-16 save creates a
readable backup named for the source version. See
[ADR-036](../architecture/adr/036-pcm-audio-document.md) for interval and
frame-edit semantics.

## Verification

- `tests/audio_tests.cpp` checks malformed import rejection, exact retained
  bytes, a cue at sample 2002, atomic undo/redo, nondestructive clip edits,
  format-16 save/reopen, rational 24/24000/1001 positions and overlapping
  44.1/48 kHz sources in one stereo mix.
- `tests/storage_tests.cpp` loads a format-15 scene and verifies `.pre-v15.bak`
  after its first format-16 save.
- `tests/export_tests.cpp` imports an actual WAV through `EditorController`,
  samples the visible cue, edits, saves/reopens and rejects a missing file.
  It checks exact 24/fractional WAV lengths, cancellation preservation and
  byte-identical output before/after project reopen.
- Local macOS `build/locked`: 148/148 CTest entries pass. The native
  `--hm10-smoke` loaded the Qt Quick audio timeline, checked its cue sample,
  captured and visually inspected `build/hm10-audio-smoke.png`, exported an
  exactly sized WAV with its cue at the correct sample, then undid import.
  The pre-existing HM-07 native dashboard smoke still passes.

## Open contract

HM-10 remains incomplete. Device-backed playback, an audio-driven playhead,
seek/loop/device-loss recovery, production-quality rate conversion,
real-time mixing, waveform pyramids, frame scrubbing, repeat and a full ten-minute
audiovisual drift run are pending. Miniaudio remains a registered candidate,
not an adopted playback dependency in this subset.
