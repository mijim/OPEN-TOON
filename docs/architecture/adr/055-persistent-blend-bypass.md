# ADR-055 — Persistent layer blend bypass

Status: experimental HM-12 subset, 2026-09-29.

## Decision

Format 29 saves `blendBypassed` on a Drawing or Part beside its retained
`blendMode`. With a non-Normal mode bypassed, the derived graph uses a typed
`BypassBlend` image node with the same two image inputs as the original blend
node. It composites with Normal source-over while keeping the selected mode
for later re-enabling. Both Display and Write evaluate this node.

The Nodes checkbox edits an unlocked Drawing or Part through one undoable
document command. It is disabled for Normal, where bypass has no visible
effect. Formats 1–28 load with bypass off, and the first save of an older
project retains a readable source-version backup. Invalid bypass owners are
rejected during document validation.

## Evidence and limits

Fractional overlap checks compare bypass with Normal pixels, including Write,
undo/redo and save/reopen. A format-28 migration test checks default state,
backup and required format-29 metadata. Native Qt Quick smoke clicks the
checkbox on an Add-blended painted cutter source and verifies output,
undo/redo, saved state and re-enabling the retained Add mode.

The graph remains derived from layer order. Arbitrary graph connections,
groups and general node bypass remain in the HM-12 contract.
