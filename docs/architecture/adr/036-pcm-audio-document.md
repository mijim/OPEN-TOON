# ADR-036 — Format-16 PCM audio assets and scene clips

Status: working HM-10 subset, 2026-09-29.

## Contract

Format 16 stores original mono or stereo PCM16 RIFF/WAVE bytes as immutable,
content-addressed project resources. Each audio asset records its source name,
sample rate, channel count and sample-frame count. The WAV header and chunk
bounds are validated before import and again when loading a document; the
recorded metadata must match the source. An asset is limited to 128 MiB; a
document supports up to 512 MiB of audio, 64 assets and 1,000 clips. Unsupported or corrupt
files fail before they can create an empty clip. Formats 1–15 load with empty
audio collections. The first format-16 save of an older project creates a
readable source-version backup such as `.pre-v15.bak`.

A clip has a stable ID, asset ID, start frame, half-open source-sample range
`[inSample, outSample)`, and linear gain in `[0, 4]`. Clip start is an integer
scene frame; the imported source retains its original sample rate and samples.
For scene frame `f >= start`, the source interval starts at
`inSample + floor((f - start) × rate.denominator × source.sampleRate /
rate.numerator)`. The next scene frame gives the exclusive end, clamped to
`outSample`. This uses the same rational `FrameRate::sampleAt` model as the
rest of the document. Waveform drawing uses the maximum absolute amplitude in
that interval, so a known cue sample stays aligned at 24 and 24000/1001 fps.

Import, placement, trim, gain and clip removal are ordinary transactional
commands with atomic undo/redo. Frame insertion shifts clip starts at or after
the insertion; earlier clips keep their start and source range. Frame removal
deletes clips whose start lies in the removed interval and shifts later starts;
it does not time-stretch earlier clips. These semantics are deliberately
bounded until the audio clock and richer timeline editing are implemented.
The clip's visible end is the first scene frame whose rational source-sample
offset reaches its exclusive `outSample`, clamped to scene duration. Timeline
dragging previews an integer-frame start without changing the document and
commits one placement command on release; Escape cancels it.

The source WAV remains part of the project after a clip is removed, allowing
future reuse. Media resources share immutable buffers across document undo
snapshots. No callback or audio device is created by this format change.

The timeline's derived peak index uses 256-source-sample maximum leaves and a
range-max tree. Queries scan partial first/last leaves exactly and combine
complete leaves from the tree, so a one-sample cue remains aligned at any
frame width. It owns a shared immutable WAV buffer, never enters the saved
format, and is rebuilt if a scene replaces an asset with the same ID.

## Offline mix and delivery

The headless `AudioMixPlan` borrows validated immutable source buffers from a
document snapshot. Its output is 48 kHz stereo PCM16. For each output sample,
the plan converts elapsed output samples to source-sample position with
integer quotient/remainder arithmetic, uses linear interpolation between
adjacent source samples, duplicates mono into both channels and sums every
active clip after its linear gain. The final sum is clamped to PCM16. Each block
is addressed by absolute output sample, so no floating-point clock is
accumulated between blocks. Higher-quality sample-rate conversion remains a
separate qualification need.

The Qt media adapter writes a standard RIFF/WAVE header and blocks to a
`QSaveFile` from an immutable scene snapshot. The exported sample count is
`FrameRate::sampleAt(document.duration, 48000)`. A file beyond the RIFF 4 GiB
limit is rejected. Cancellation discards the temporary file and preserves an
older destination. Audio export shares the existing background export status
and cancellation control with PNG export, so only one job runs at a time.

## Evidence and limits

Domain tests reject malformed input without mutation, check exact source
bytes and cue peaks, edit/undo/redo, save/reopen and rational frame mapping.
The format-15 migration test verifies a readable `.pre-v15.bak` backup. The
editor test imports a real WAV file, edits a clip, samples its waveform and
reopens it. Two-source tests cover overlapping clips, gain, 44.1-to-48 kHz
linear conversion and stereo output. Editor export tests check exact 24 and
24000/1001 lengths, cancellation preservation and byte-identical mixes after
reopen. Native Qt Quick smoke inspects the audio row and control panel,
captures `build/hm10-audio-smoke.png`, checks the exported cue, and undoes import.

The experimental desktop now uses the same `AudioMixPlan` for a miniaudio
device callback and offline export. See [ADR-037](037-miniaudio-preview-clock.md).
Hardware clock qualification, higher-quality rate conversion, audible
scrubbing and clip repeat remain HM-10 work.
