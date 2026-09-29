# ADR-065: Selected timeline range playback

Status: accepted for the HM-10 subset, 2026-09-30.

## Context

The timeline selects a half-open frame interval for editing and exact WAV
export. Preview previously played the entire scene even when an animator
needed to inspect that interval repeatedly.

## Decision

The toolbar's Range mode uses the selected `[first, end)` frame interval when
Play starts. The immutable audio device converts both boundaries through the
scene's rational frame-to-48-kHz-sample mapping. The callback reads only
samples in `[sampleAt(first), sampleAt(end))`; Loop returns to the first sample
and Play once stops at the end sample with the last included frame visible.
Silent preview uses the same frame bounds. Starting outside the range jumps to
its first frame. A seek outside the active range stops playback before moving
the playhead. Invalid or active-device range changes reject before mutation.

Range mode is view state, clears when a different scene opens, and does not
alter document, source WAV or exported audio. The interval is captured at Play;
changing the selection may stop playback through its normal seek behavior.
The callback remains bounded and does not acquire a document lock. No project
format change is needed.

## Verification

Null-device tests check invalid bounds, exact integer and 24000/1001 end
samples, final frame and loop wrap. Controller tests cover silent range playback
and document preservation.
Native Qt Quick smoke clicks Range on a real imported WAV and verifies first
and last frames, one-pass stop and unchanged scene state.
