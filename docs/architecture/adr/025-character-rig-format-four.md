# ADR-025 — Typed character layers and format-4 substitutions

Status: accepted for the bounded HM-01 property contract and an initial HM-03
rigging subset, 2026-09-23. This extends [ADR-023](023-harmony-foundation-contracts.md)
without changing the full Harmony Moment acceptance profile.

## Decision

Format 4 adds `kind`, `role` and `variants` to each layer. `kind` is one of
Drawing, Character, Peg or Part. Stable document-global layer IDs remain the
property and hierarchy identities. A Part has a role, and its named
substitutions map stable drawing IDs to labels owned by that Part. The existing
half-open exposure intervals are the first held drawing-choice track; an
exposure on a Part must reference one of its substitutions. Character roots and
pegs own no drawing exposures. The list of variants preserves alternatives even
when none is active on the timeline. Renaming a variant changes only its Part
label, so a deliberately linked drawing resource does not rename another part.

The typed transform property address checks Character/Peg/Part identity before
reading or editing the eight existing rest, authored-key and evaluated
channels. `Layer` remains a compatibility entity kind for current UI callers.
This avoids a second evaluator or a wholesale rewrite of full-pose keys.
Substitution selection uses a discrete drawing ID command, never a floating
point transform value. All UI actions use `Session::apply` and one undo entry.

On the first save of a schema 1–3 project, SQLite makes an independent backup
named `.pre-v{source-version}.bak` before the migration transaction. A collision
gets a numeric suffix. Format-3 JSON layers decode as Drawing with no rig
metadata. New revisions encode format 4 and the database `user_version` becomes
4 in the same transaction. Readers limited to format 3 reject the revision
version and database version instead of dropping the new fields. Failure before
commit keeps the original loadable. No new library was adopted.

## Geometric limits and current interaction

The inspector creates a character from an unlocked root drawing, can place a
peg above a part, attach another registered drawing to a character/peg, edit
its role, and change a rest pivot while retaining artwork position. Reparenting
within a character preserves the complete local affine image and opacity for
the rest pose. It rejects singular matrices, introduced shear, opacity
impossible to preserve, cross-character moves, animated ancestors and direct
reparenting of an already animated part/peg. Permanent pivot placement and
attaching a root drawing also require that layer to be unanimated. This avoids
claiming that only key endpoints preserve the in-between artwork motion.
Animation under moving parents and shear-preserving transform storage need a
later contract. The first root and new peg are identity transforms; creating
them therefore does not jump registered artwork.

The compact inspector creates blank or copied substitution drawings, names and
chooses them at the playhead, and removes an option with atomic replacement of
its held intervals. Exposure intervals remain the selector track. New drawing
commands and timeline paste register drawings as substitutions when targeting
a Part. The renderer and exporter already read `drawingAt`, so they evaluate
the same held choice without an independent UI renderer.

This is a rigid cut-out foundation, not full HM-03. Thumbnail previews,
coordinated view sets, independent instance remapping, pose masks and animator
controls remain open. Deformers, audio and node compositing are unaffected.
