# ADR-037 — Experimental miniaudio playback clock

Status: experimental HM-10 subset, 2026-09-29.

## Decision

The desktop's bounded PCM16 preview uses miniaudio 0.11.25, pinned by the
Conan recipe revision in `conan.lock`. The media adapter owns one playback
device and an immutable document snapshot. It constructs the document's
`AudioMixPlan` before starting the device. The callback reads only that plan,
renders into bounded stack scratch memory and publishes a sample cursor with
an atomic compare/exchange. It does not allocate, lock the document, access
Qt, or mutate media. The same rational sample mapping and mix algorithm
serves headless WAV export.

The callback submits 48 kHz stereo PCM16. Scene frame `f` starts at
`FrameRate::sampleAt(f, 48000)`; the scene loops at the exclusive
`sampleAt(duration, 48000)` boundary. Seeking swaps the cursor atomically;
an in-flight callback can finish its current buffer but cannot overwrite a
new seek position. The UI playhead reads the submitted-sample cursor instead
of advancing a second wall clock. An edit stops playback before publishing a
new document revision. Device interruption or reroute stops playback and
reports it; a failed output open falls back to silent visual preview.

The adapter counts callbacks whose processing time exceeds their output
period and retains the maximum callback duration. The desktop counts
playhead frames skipped by its 8 ms timer and exposes both through playback
diagnostics; nonzero counts are reported when preview stops. A mixer callback
over its period is a scheduling warning, not a device underrun measurement.

Timeline traversal opens the same adapter in scrub mode. A requested frame
starts an 80 ms sample fragment, capped at scene end; remaining callback
frames are silent. Moving to another frame atomically replaces the fragment
cursor, and release stops the device. This uses the exact scene sample map,
not a second timer. A drag that moves an audio clip itself does not scrub;
it remains a document placement gesture.

## Evidence and limits

The null backend test verifies start, advance, seek, bounded scrub, stop and
snapshot preservation. Native Qt Quick events verify that timeline travel
starts, changes and ends a scrub without document mutation. A 600-second
24000/1001 scene with overlapping 48 and 44.1 kHz
clips measures p95 0.014 ms to mix a 1024-frame block against a 21.33 ms
output period on the M1 Pro Release profile. Offline WAV tests verify the
same cue and exact rational sample counts. The selected license option is
MIT No Attribution (MIT-0); the package has `licenses/LICENSE`.

The cursor represents frames submitted to the backend, not a calibrated
speaker presentation timestamp. Device queue latency, real hardware
underruns, device replacement, audible fragment quality, 10-minute audiovisual drift and other host
profiles still need measurement. HM-10 remains in progress. If device
qualification fails, retain the offline PCM WAV mix and silent visual
preview while evaluating Qt Multimedia behind this adapter boundary.
