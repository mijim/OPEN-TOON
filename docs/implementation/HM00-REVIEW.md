# HM-00 reference review and closure

On 2026-09-22 the project owner accepted the artistic review of the **Clockwork
Hello** reference material and shot rubric in this task: “la revision artistica
esta bien por mi parte.” The approval covers the proposed character design, five
reference stills and six-beat scripted target. It releases the HM-00 reference
gate for the dependent engineering slices.

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
