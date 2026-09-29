# ADR-043 — Explicit drawing opacity in the derived composition graph

Status: experimental HM-12 subset, 2026-09-29.

## Decision

The derived graph inserts a typed Image → Opacity → Image node for a Drawing
or Part whose saved setup or pose keys contain opacity other than one. It
uses the existing animated layer-opacity property; no second control or new
project field is introduced. The compact Nodes panel edits that property on
the selected drawing. Source images are evaluated without their own opacity
and the node multiplies premultiplied color and alpha by the evaluated value.
Ancestor opacity remains in the source evaluation. A cutter takes its source
after this opacity node, so partial source opacity changes matte coverage;
target opacity is applied before the matte. Display and Write use the same
graph. The older direct renderer remains the LegacyQt path for scenes that
do not request a graph output.

No opacity node is inserted for a layer that is fully opaque at every saved
setup/key pose. This preserves the old graph topology and pixels of existing
opaque scenes. The node is derived from format-19 data and is not serialized
as a second authority. Undo, save/reopen and project migration retain the
existing transform semantics.

## Evidence and limits

A one-pixel fractional fixture evaluates source and target opacity through
Inside and Outside cutters, checks Display/Write and frame-key changes, then
reopens to identical pixels. The native Qt Quick smoke opens a saved scene,
shows the opacity control, edits the cutter source, and undoes the result.
The full macOS suite passes 165 CTest entries. A general editable graph,
group ports, custom node parameters and joint recipes remain HM-12 work.
