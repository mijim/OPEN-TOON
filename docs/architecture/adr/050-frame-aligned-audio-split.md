# ADR-050 — Frame-aligned split of a PCM clip

Status: experimental HM-10 subset, 2026-09-29.

## Decision

`Split` divides one placed clip at the playhead into two clips sharing the
same immutable WAV asset. It keeps the left clip ID and assigns the right a
new stable ID. The right clip starts at the selected scene frame; its source
in-sample is the old in-sample plus the exact difference between the
frame-to-sample positions of the selected frame and the old start. The left
out-sample meets that in-sample. Gain and mute are copied. A fade-in remains
on the left and a fade-out remains on the right. The command creates one undo
entry and never changes the source bytes.

The current bounded operation accepts a 48 kHz PCM16 source with one repeat,
an interior frame and a cut outside both fade intervals. The canonical WAV
mix and export run at 48 kHz; these conditions make the two new placements
sample-for-sample identical to the original in that output. Other source
rates, repeated clips, cuts inside a fade and boundary frames reject before
publication. No project format field changes. A device with a different
output rate may have different resampler edge samples near a cut; its audible
quality remains part of HM-10 hardware qualification.

## Evidence and limits

Domain tests compare the complete before/after stereo PCM mix at 24 and
24000/1001 fps, original WAV bytes, clip metadata, undo/redo and project
reopen. Negative cases cover boundary frames, repeats, a cut through a fade
and a 44.1 kHz source. Native Qt Quick smoke clicks Split at an interior
playhead frame, compares the canonical mix and undoes the change. More
general nondestructive split and cross-rate continuity remain open.
