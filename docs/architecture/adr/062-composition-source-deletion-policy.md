# ADR-062 — Explicit composition source deletion policy

Status: experimental HM-12 subset, 2026-09-29.

## Decision

A Drawing or Part source card exposes **Delete source…** with two explicit
choices. **Protect references** deletes only when no external cutter user or
composite group depends on the source. **Disconnect references
and delete** clears external cutter bindings and their inversion/bypass flags,
removes intersecting group boundaries, then deletes the Drawing or the complete
Part branch. Other layers retain their order and artwork. A Drawing with child
layers still requires reparenting first; locked sources or cutter users reject
the operation.

The application performs the chosen policy in one validated Session command.
Failure leaves the original document intact. Undo/redo restores or removes the
source, cutter references and group boundaries together. No format change is
needed; the existing serialized layer and group fields represent the result.

## Evidence and limits

One-pixel Drawing and grouped Part fixtures check both choices, locked-user
rejection, alpha/result pixels, undo/redo and save/reopen. Native Qt Quick
smoke clicks both choices on a referenced source and verifies the resulting
cutters after reopen. Deletion of arbitrary processing nodes, user-wired
reconnection and a general graph layout remain open under NOD-001.
