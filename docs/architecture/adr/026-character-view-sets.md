# ADR-026 — Character view sets and format-5 persistence

Status: accepted for the bounded HM-03 rigid character workflow, 2026-09-23.
Extends [ADR-025](025-character-rig-format-four.md); HM-03 acceptance remains open.

## Decision

Format 5 adds `views` to Character layers. A view has a document-global stable ID,
unique name within its character and a list of `(part ID, drawing ID)` choices.
Each choice must reference a Part in the same character and a substitution
registered on that Part. A captured view contains every Part that currently has
a drawing at the requested frame. Applying a view requires an exact current-Part
match and unlocked targets, validates every choice first, then writes held
exposure intervals in a single Session command. Changing a transform is never
part of switching a view. An option referenced by a view cannot be removed
until its view references are changed or removed.

Duplicating a character copies the complete descendant hierarchy and every
referenced drawing, stroke and view identity. References among the copies are
remapped; source artwork and views remain independent. The new root is offset
in its rest and keyed position so it can be found on the canvas. This is a
snapshot copy, not a linked instance or reusable template contract.

Structural rig edits keep saved views referentially valid. Adding a registered
Part inserts its initial choice into every existing view; batch assembly applies
that rule to exposed, unlocked root drawings in one command. Copying a Part or
Peg branch remaps descendant layer identities and extends each view with the
corresponding copied choices. An independent copy gives drawings and strokes
new IDs; an explicit linked-artwork clone shares drawing IDs while retaining
separate layers, exposures and view choices. Removing a branch removes its
view choices. Detaching a Part or dissolving a Peg keeps the visible rest pose
where a representable transform exists, and rejects animated or singular
cases before publication. These operations use the existing format-5 fields.

A view can apply to a half-open `[start,end)` range without changing adjacent
exposures. The selected Part's current drawing can update one view choice
without recapturing every other Part. View order and active-view navigation
are separate from timeline keys: reordering is a document command, while
navigation is presentation state.

The Properties panel shows each Part's alternatives as compact image tiles.
The tile is generated from the actual scene renderer and cached by document
revision and drawing ID. The cache is presentation state, never project data.
View-set capture, application, rename, update, duplication and removal live in
the same inspector. These actions still pass through Session transactions.

Format-4 projects decode with empty view lists. Their first successful format-5
save creates a `.pre-v4.bak` source-version backup and upgrades SQLite's
`user_version` in the save transaction. Failures retain the readable old file;
future-version readers reject the format. No new dependency was adopted.

## Limits

Views contain drawing substitutions only. Masked transform poses and controls
belong to HM-07; Quick Rig belongs to HM-08. The inspector thumbnails are
on-demand rather than a precomputed image library. Character duplication is
independent, but linked instances and portable asset templates remain HM-09.
Reparenting under animated ancestors and animator acceptance of the full
19-part workflow remain open in HM-03. An automated 19-part scene now verifies
typed parts, view switching, independent copying and save/reopen; it does not
replace that artist review.
