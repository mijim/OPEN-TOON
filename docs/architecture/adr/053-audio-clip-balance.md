# ADR-053 — Per-clip stereo balance

Status: experimental HM-10 subset, 2026-09-29.

## Decision

Format 27 adds a finite per-clip `balance` value in [-1, 1]. Zero preserves
the historical stereo mix exactly. Negative values attenuate the right output
channel and positive values attenuate the left; the favored channel retains
its existing gain. For a mono source this positions its identical output
channels; for a stereo source it acts as balance without crossfeed or a change
to source bytes. The gain factors are calculated within the existing immutable
mix plan and used by device preview, scrub and WAV export.

The Audio panel provides a compact numeric field: -1 is left, 0 center and
+1 right. One validated document command applies the setting with undo/redo.
Duplicate and split retain it. Format-26 clips load centered and the first
format-27 save preserves a readable `.pre-v26.bak`. Missing or out-of-range
current-format values reject before document publication.

## Evidence and limits

Exact PCM tests check center, halfway right and full left channel values,
copy independence, validation rollback, undo/redo, original WAV bytes and
save/reopen. A migration test checks the default, backup and malformed schema
rejection. Native Qt Quick smoke enters a balance value in the inspector,
checks the output channel and undoes it. This is channel balance, not spatial
panning or a per-track bus architecture.
