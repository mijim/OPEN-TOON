# ADR-063: Editable manual joint patch

Status: accepted for the HM-12 subset, 2026-09-30.

## Context

An arm behind a torso may need a small amount of arm ink over the torso at
the joint. Existing order changes move the entire arm; cutter bindings clip
an image but do not provide that foreground overlap.

## Decision

The Nodes inspector creates a private copy of a visible leaf Part in the same
character as the selected target Part. The copy is inserted above the target's
contiguous composite group (or target when ungrouped). It receives independent
drawing and stroke IDs and retains exposure timing, keys, transforms, mesh
bindings, and registered character view and pose choices. The source Part and
existing group membership remain intact. Creation is one undoable document
command. The animator edits or erases copied ink to define the patch.

The recipe uses existing Part, drawing and composite order fields; the project
format does not change. It does not infer a patch silhouette, repair bent
contours automatically, or promise seam quality across arbitrary poses.

## Verification

An alpha-half arm over an opaque torso is checked at two keyed positions for
opaque output and visible overlap. Domain and editor tests check independent
artwork, view/pose/mesh closure, grouping, undo/redo and save/reopen; native Qt
Quick smoke clicks the recipe and repeats pixel, undo and reopen checks.
