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
Reparenting under animated ancestors and the full 19-part animator acceptance
journey remain open in HM-03.
