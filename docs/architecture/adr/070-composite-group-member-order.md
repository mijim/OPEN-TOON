# ADR-070 — Direct composite group member order

Status: adopted for the experimental desktop editor, 2026-09-30.

A Drawing or Part card dragged onto another member of the same contiguous
composite group changes their saved order. An ordinary drag places the source
in front of the target; Shift places it behind. The document command changes
both the Drawing/Part layer sequence and the group's member-ID sequence in
one transaction. The existing group ID, name, bypass state and typed ports
remain stable. The same layer-order rule already used outside groups defines
the resulting composite image.

Both cards must belong to the same group. A missing target, unchanged position
or locked member rejects the edit without a revision. The command operates on
a validated copy, so a resulting invalid Part hierarchy or cutter reference
also rejects without publishing partial state. Undo/redo and project reopen
preserve the order and pixels. The existing format-31 member list already
records this order; no format change or migration is needed.

This covers direct internal order editing in contiguous groups. General
subgraph layout, arbitrary wiring, nested groups and published external ports
remain separate HM-12 work.
