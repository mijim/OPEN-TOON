# ADR-045 — Visible cutter source

Status: experimental HM-12 subset, 2026-09-29.

## Decision

Format 21 adds `paintMatteSource` to Drawing and Part layers. It defaults to
false, preserving the source-only cutter behavior of formats 1–20. When a
layer is referenced as a cutter and this flag is true, its evaluated image
feeds both the target's fractional matte and its own ordered `Over` node.
It paints once at its normal layer order, after any source opacity. Multiple
targets can share that image and the visibility choice. A source may be placed
behind or in front of its targets using existing layer-order commands.

The control appears on the selected target in Properties and Nodes, and edits
the referenced source through one document transaction. A locked source or
target cannot be changed. Source reference validation, deletion guards and
the no-chained-cutters rule remain. Target bypass removes its matte dependency;
the source still paints if this flag is enabled. A first format-21 save of an
older project preserves a readable source-version backup.

## Evidence and limits

A fractional-alpha render fixture checks hidden and painted source pixels,
Display/Write parity, serialization and restoration. The typed graph test
checks that a painted source reaches Write even while its target is bypassed.
Format-20 migration checks default-off behavior, a saved-on choice and its
readable backup. Native Qt Quick smoke checks the control, pixels, undo/redo,
save/reopen and disabling. Manual arm/eye overlap recipes and a general
editable graph remain open.
