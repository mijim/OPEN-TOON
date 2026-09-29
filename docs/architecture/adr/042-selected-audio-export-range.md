# ADR-042 — Exact selected-frame PCM WAV export

Status: experimental HM-10 subset, 2026-09-29.

## Decision

The audio writer accepts a half-open scene-frame range `[start, end)` in
addition to full-scene export. It maps both frame boundaries independently
through `FrameRate::sampleAt(frame, 48000)`. The output contains exactly the
mixed samples in `[firstSample, endSample)`, with a RIFF/WAVE length derived
from their difference. A selected range therefore has the same PCM bytes as
the corresponding slice of a full-scene export, including at 24000/1001 fps.

The desktop offers **Export selected PCM WAV range** for a nonempty timeline
selection. It launches the same immutable-snapshot background job as full
export, reports progress over the selected samples and commits through
`QSaveFile`. Invalid ranges fail before a file header is written. Cancellation
discards the temporary output and preserves any existing destination. The
scene, source WAV resources and clip offsets are never rewritten.

The range affects audio only. A synchronized PNG/WAV/manifest delivery job
remains an HM-14 contract after the audio and compositor prerequisites pass.

## Evidence

Integration tests compare the selected PCM data byte-for-byte with a full
mix slice at 24 and 24000/1001 fps, reject an empty/reversed range without
writing output and verify the controller's native asynchronous export.
The HM-10 Qt Quick smoke exports a two-frame slice with its known cue at the
corresponding output sample.
