# HM-00 reference review and closure

On 2026-09-22 the project owner accepted the artistic review of the **Clockwork
Hello** reference material and shot rubric in this task: “la revision artistica
esta bien por mi parte.” The approval covers the proposed character design, five
reference stills and six-beat scripted target. It releases the HM-00 reference
gate for the dependent engineering slices.

On 2026-09-28 the owner judged the original geometric character too crude for
deformation review. Its design approval does not apply to the replacement art.
The original 19-part/41-image asset set and five reference stills were redrawn
as a more expressive color toon while retaining the accepted shot, timing,
registration and provenance contract. Technical reference tests pass on the
replacement; visual approval of the new design remains open. The earlier
acceptance records the historical HM-00 rubric decision rather than an artistic
endorsement of this later revision.

The owner then rejected that replacement as still too generic and supplied
Toon Boom rig examples for visual direction. A second independent redraw uses
a hoodie silhouette and softer part joins while retaining the same registered
roles, variants and shot rubric. The owner has not yet approved this redraw.

The reproducible input, original-art provenance and exact time variants are
documented in [the fixture guide](../../tests/fixtures/harmony-moment/README.md).
The [foundation ADR](../architecture/adr/023-harmony-foundation-contracts.md)
fixes identity, space, time, color, transaction and migration boundaries. The
`Original registered character parts survive format-3 save and reopen` CTest
checks the 19-part rigid baseline, first-frame pixels and rational sample boundary.
It does not test a built rig, animation or audio playback.

The final **second-animator** review of the working 20-second shot, independent
reuse, correction time, hidden state and failure recovery remains an explicit
HM-15 acceptance gate. Owner approval here does not satisfy that gate or complete
P00–P11.
