# ADR-056 — Owner selection from derived composition nodes

Status: experimental HM-12 subset, 2026-09-29.

## Decision

`GraphNode::layer` identifies the document layer that owns a derived node's
editable setting. A cutter source node points to its source Drawing or Part;
Inverse matte, Apply matte and composite nodes point to their target. Existing
Drawing, Opacity and bypass nodes retain their prior owner identity. Clicking
a node card selects that layer in the editor, so the compact inspector edits
the same source or target shown in the graph.

The graph validator accepts an owner on these node kinds only when it names a
Drawing or Part. Owner metadata is regenerated from the document on every
graph build; it is not a second persistent graph or a new project format.
Port typing, compositing order, output pixels and saved layer references remain
document-owned.

## Evidence and limits

Graph tests check source/target ownership and reject a dangling composite
owner. Native Qt Quick smoke clicks Drawing, Cutter, Apply matte and
Composite cards, checking the selected layer and preview pixels. The full
macOS CTest suite and native HM-12 smoke pass after the change. Arbitrary node
creation, wiring and group ports remain open.
