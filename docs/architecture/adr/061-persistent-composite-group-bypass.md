# ADR-061 — Persistent composite group bypass

Status: experimental HM-12 subset, 2026-09-29.

## Decision

Format 32 stores a `bypassed` boolean on each named composite group. Format-31
groups load with bypass off; an older file claiming active group bypass is
rejected. Its first format-32 save retains a readable `.pre-v31.bak` backup.

The derived Group output has two typed image inputs: the image at the Group
input boundary and the fully composed internal image. It forwards the first
when bypassed and the second when active. Layer image and cutter source nodes
remain in the graph, so a source inside a bypassed group can still mask a
target outside it. Bypass changes output, not document ownership, animation,
or the saved settings of individual members. The graph may still evaluate
internal inputs; this control makes no performance promise.

Nodes exposes **Bypass group** for a selected member and Alt-click on the
Group output card. One transactional command saves the state with undo/redo;
locked group members reject edits. Re-enabling restores the original output.

## Evidence and limits

One-pixel graph and renderer checks compare active and bypassed Display,
Write and Group output pixels, including a matte-only source inside a bypassed
group used by an external target. Controller and native Qt Quick tests cover
toggle, undo/redo and reopen. Storage tests cover format-31 migration, backup
readability and older-schema rejection. General user-wired graph bypass and
nested or reusable groups remain open.
