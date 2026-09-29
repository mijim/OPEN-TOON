# ADR-059 — Persistent layer composite bypass

Status: experimental HM-12 subset, 2026-09-29.

## Decision

Format 30 stores `compositeBypassed` on Drawing and Part layers. A derived
`BypassComposite` image node forwards the previous composite without painting
that layer. The layer's source image and opacity nodes remain available to
cutter consumers and diagnostic previews. This preserves a painted cutter's
matte contribution while its own ink is bypassed. It differs from visibility,
which can make a cutter invalid, and from blend bypass, which still paints the
layer with Normal source-over.

The Nodes toolbar and Alt-click on Composite or Bypassed composite cards issue
one validated document command. Undo, redo, Display, Write and export use the
same saved state. The node's image input is only the preceding composite, so
editing an otherwise unused bypassed source does not invalidate Write. If the
source is used as a cutter, its consumers retain the dependency.

Formats 1–29 load with composite bypass off. The first format-30 save of an
older file retains a readable source-version backup. Only Drawing and Part
layers may save this flag.

## Evidence and limits

Graph, one-pixel cutter/render and storage tests check dependency isolation,
fractional matte reuse, Display/Write parity, undo/redo, migration and reopen.
Native Qt Quick smoke clicks the control and bypassed card, checks pixels,
undo/redo and reopen. The complete arbitrary graph bypass contract, groups and
part-overlap recipe remain open.
