# ADR-060 — Contiguous composite groups

Status: experimental HM-12 subset, 2026-09-29.

## Decision

Format 31 stores named `compositeGroups` with stable IDs and an ordered list of
two to 256 Drawing or Part IDs. Members must be adjacent in drawing order and
cannot belong to two groups. A group may cross non-painting Character/Peg rows;
it does not change the character parent hierarchy. Commands reject moving or
deleting a member if that would leave invalid group membership. Ungroup first.

The derived graph inserts typed `GroupInput` and `GroupOutput` image ports
around the group's ordered composite segment. Both ports forward the same
image bytes and bounds. Source image nodes and external cutter dependencies
remain available, including a matte-only member inside the group. Grouping,
ungrouping and renaming are transactional document commands. In Nodes,
Shift-click two Drawing cards to group their inclusive span, edit the compact
group name, or use Ungroup on a selected member. Reopening restores group IDs,
names, members and ports.

The renderer's isolated source copy clears groups before rendering, preventing
recursive graph entry. Older formats load with no groups. The first format-31
save of a format-30 project retains a readable `.pre-v30.bak` source backup.

## Evidence and limits

Graph and pixel tests cover typed ports, adjacency and overlap rejection,
matte-only sources, Display/Write parity and pixel-identical grouping.
Application and native Qt Quick tests cover grouping, renaming, one-step
undo/redo, save/reopen and ungrouping. Storage tests cover format-30 migration,
backup readability and unsupported older-schema group rejection. The locked
macOS suite passes 194/194 CTest entries.

These ports encapsulate one contiguous composite segment. Nested groups,
arbitrary source-node capture, published external bindings, reuse in another
graph and general user-wired ports remain open under NOD-004 and HM-12.
