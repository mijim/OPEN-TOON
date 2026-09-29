# ADR-052 — Frame-aligned audio edge trim

Status: experimental HM-10 subset, 2026-09-29.

## Decision

A single-pass 48 kHz PCM clip can be trimmed by dragging either of its middle
timeline edge handles. The left handle changes both the scene start frame and
the source in-sample by the same exact rational frame-to-48 kHz sample delta.
The right handle changes only the source out-sample. One source sample must
remain. An attempted edge outside the scene or the original WAV rejects the
whole edit. Existing source-sample fades clamp to the shortened interval, as
they do for numeric trim. The original WAV and all other clips stay intact.

The drag shows an edge preview and commits one transactional undo step on
release. Escape cancels. This uses the existing audio clip schema; the
function's exact 48 kHz source requirement is the same bounded clock profile
as the frame-aligned Split command. Other source rates and repeated clips
retain numeric sample trim until cross-rate and cycle-edge semantics are
specified. Device output at another rate can resample differently at a new
clip boundary; hardware audible quality remains an HM-10 gate.

## Evidence

At 24 and 24000/1001 fps, domain tests compare every surviving canonical
48 kHz PCM sample before and after left/right trims, exact source and scene
coordinates, undo/redo, out-of-WAV rollback and save/reopen. Native Qt Quick
smoke drags both timeline edges, checks sample positions, PCM continuity and
one-step undo.
