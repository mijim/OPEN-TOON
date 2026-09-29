# ADR-041 — Bounded band-limited sample-rate conversion

Status: experimental HM-10 subset, 2026-09-29.

## Decision

The immutable `AudioMixPlan` precomputes one 1,024-phase, 32-tap
Blackman-windowed sinc table for each distinct source rate different from the
output rate. The cutoff is 90% of the smaller of the source and output
Nyquist limits, expressed in source-sample units, and each phase is
normalized for unity DC gain. Tables are built before playback and shared
among clips with the same rate. The device callback reads them without
allocation, locks, trigonometry or document access.

Absolute output-sample addressing and integer quotient/remainder conversion
continue to determine the source position. FIR taps traverse a clip's
sample-contiguous repeats; before the first sample and after the final
sample, they hold the edge sample. The mixer still skips times outside the
clip's scene interval, so filtering does not start a clip early. Equal-rate
sampling preserves its direct sample path.
Output remains 48 kHz stereo PCM16 for desktop preview and offline export.
No original WAV bytes or saved clip timing change.

## Evidence and limits

At 96-to-48 kHz, a 30 kHz input measured 0.000089 output RMS against a
0.02 maximum, while a 1 kHz input retained 0.561 RMS for a 0.794 peak
source. Tests also check identical output across arbitrary block boundaries
and a repeated cue at the same source-sample position on each loop. Two
simultaneous 96 kHz tracks measured 0.232 ms p95 for a 1,024-frame callback
on the local M1 Pro Release build, against a 21.33 ms output period.

At 8-to-48 kHz, the same kernel reconstructs a 3 kHz source tone within its
passband, and split output blocks match one whole block. This replaces the
former linear upsampling attenuation near the source Nyquist limit.

This is a bounded real-time anti-alias path, not a mastering-grade sample
rate converter. Full rate-ratio/quality sweeps,
audible device inspection and hardware presentation timing remain open.
