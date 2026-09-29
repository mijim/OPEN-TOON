# ADR-047 — Saved per-clip audio mute

Status: experimental HM-10 subset, 2026-09-29.

## Decision

Format 23 adds a `muted` Boolean to each audio clip. Formats 1–22 load with
`muted = false`; a first save of an older project retains a readable
source-version backup. Muting is a transactional edit with one undo step. It
does not change the original WAV resource, clip placement, trim, repeats,
fades or gain. A duplicated clip copies the current mute value and can then
be toggled independently.

The immutable audio mix plan validates every clip, then omits muted clips
before preparing source pointers and rate-conversion kernels. Playback,
fragment scrub, full WAV export and selected-range export use this same plan.
The timeline retains the source waveform as a dim reference and labels the
clip `Muted`; it does not present that trace as audible output.

## Evidence and limits

Tests mix two clips sharing one source and check that muting one removes only
its contribution, muting both yields silence, undo/redo restores exact PCM,
and WAV bytes remain unchanged. A format-22 project loads unmuted, saves a
muted edit and retains a readable `.pre-v22.bak`. Native Qt Quick smoke clicks
the Mute control and undoes it. Hardware output and device-loss qualification
remain HM-10 work.
