# ADR-064: Playback loop boundary

Status: accepted for the HM-10 subset, 2026-09-30.

## Context

Playback previously wrapped at the scene's rational end sample with no user
choice to inspect the final frame after one pass. Silent preview and device
preview need the same visible transport behavior.

## Decision

Loop playback is view state, enabled by default. The toolbar toggle applies to
the current immutable audio device through an atomic flag and to silent
preview through the editor timer. With looping off, the device callback fills
the remainder of its output block with silence at the exact scene end sample;
the UI stops playback and leaves the last frame visible. With looping on, the
cursor returns to sample zero at that boundary. Changing the toggle never
mutates document or media. Editing still stops playback before publishing a
new revision. The project format does not change.

The callback remains bounded, allocation-free and lock-free with respect to
the document. A finished device may still be started until the UI timer stops
it, so `finished()` represents cursor state rather than hardware teardown.

## Verification

A null miniaudio backend checks the exact end sample, last frame and loop
wrap. An editor test checks automatic stop and unchanged document; native Qt
Quick smoke clicks the toolbar toggle and checks final-frame stop. Hardware
underruns and output-device latency remain outside this bounded result.
