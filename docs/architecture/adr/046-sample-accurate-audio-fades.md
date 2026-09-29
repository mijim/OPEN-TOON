# ADR-046 — Source-sample audio fades

Status: experimental HM-10 subset, 2026-09-29.

## Decision

Format 22 adds nonnegative `fadeInSamples` and `fadeOutSamples` to each audio
clip. They default to zero for formats 1–21. Durations are measured in the
original PCM asset's sample frames, after nondestructive trimming and across
the complete repeated clip. The first repeated cycle does not restart a fade.
Their sum cannot exceed the repeated clip length. Setting both values is one
transaction; shortening a trim or repeat count clamps the fade-in first and
then the fade-out to preserve that invariant. Moving the clip leaves both
durations unchanged.

The immutable mix plan applies a linear endpoint envelope after rate
conversion and before clip gain and track summation. A nonzero fade starts
at silence on the first source sample or ends at silence on the last source
sample; intermediate output samples interpolate by the exact rational source
position. One source-sample fade silences its endpoint sample. The same plan
serves device preview, scrub and full or selected-range offline WAV output.
No original WAV bytes are modified and the audio callback allocates no new
memory. The timeline waveform remains a view of the original source peaks
and saved gain, so the fade envelope itself is not drawn there yet.

## Evidence and limits

A constant PCM fixture checks exact endpoints, repeated-cycle continuity,
undo/redo, invalid sum rejection, trim clamping and serialization. A real
format-21 project migrates with zero fades, saves chosen values and keeps a
readable `.pre-v21.bak`. Native Qt Quick smoke checks fade controls, exported
sample changes, repeat alignment and undo. Hardware audible quality and
presented-frame sync remain open.
