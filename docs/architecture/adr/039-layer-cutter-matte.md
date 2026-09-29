# ADR-039 — Saved layer cutter matte binding

Status: experimental HM-12 subset, 2026-09-29.

## Decision

Format 18 adds an optional `matte` layer ID to each Drawing or Part layer.
Formats 1–17 load with no binding. The source must be a distinct, visible
Drawing or Part without another matte. A source may be shared by multiple
targets. Validation rejects missing, self-referential and chained bindings.
The application changes the binding through one transactional command, so
undo restores the previous image and document state. Removing a referenced
source is rejected until its bindings are removed.

The derived typed composition graph renders each source once, converts its
fractional alpha to a matte, multiplies the target's premultiplied channels
and alpha by that matte, and composites the result in layer order. A bound
source is excluded from the final composite. The source and target remain
ordinary editable layers; the inspector offers only valid sources and a
`No cutter matte` bypass choice. The same derived graph drives display and
write output for matte-bearing documents under either color profile.

The binding is a document reference, not a serialized arbitrary graph. An
independent character copy remaps internal source IDs. Copying a branch with
an external matte source is rejected to avoid a dangling or unintended
cross-character reference. The existing graph validates typed ports and
cycles; a general graph editor remains a separate HM-12 contract.

## Evidence and limits

Domain and render tests check invalid references, fractional coverage, source
exclusion, display/write parity, save/reopen and bypass. A native Qt Quick
smoke checks inspector visibility, binding, save/reopen and undo. Part overlap
still uses explicit layer order. Group/ungroup, arbitrary node editing,
general opacity nodes and joint-specific recipes remain open.
