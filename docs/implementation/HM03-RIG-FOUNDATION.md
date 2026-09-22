# HM-03 rigid character foundation — partial

The layer inspector can turn a registered drawing into a character root and
part, add a peg above it, attach other registered root drawings to that
character, assign a part role and set a permanent rest pivot without moving
artwork. Reparenting a part or peg within its character preserves the image
where the transforms are representable, and refuses singular/sheared or
animated-ancestor or already animated target cases. The animator can create blank or copied named
substitutions and choose a drawing at a frame. The choice holds through the
current exposure interval, is saved, undoes atomically and renders identically
after reopen. Removing an option replaces its active spans with the first
remaining option or clears them if none remains.

The Properties panel now shows renderer-backed thumbnails for named
substitutions. Artists can reorder and step through choices. A character root
stores named view sets that capture each Part's active substitution at a frame;
applying one validates the full target set and changes all Parts as one undoable
command. View sets can be updated, renamed, copied and removed. A full
character duplication remaps descendant layers, drawings, strokes and views,
so editing copied artwork cannot alter the source. These data round-trip in
format 5 with a preserved format-4 migration backup.

This supports a rigid cut-out workflow using the existing timeline and
Properties panel. HM-03 remains partial: an artist-led 19-part character
acceptance journey, validation of the thumbnail/view interaction and
animated-ancestor reparenting remain. The reparent tool cannot solve
animated-parent motion, reparent an animated child or store shear. Rest pivot
placement must occur before that layer is animated. The copy is an independent
snapshot, not a linked character asset or HM-09 template.

The next structural block adds batch assembly of exposed root drawings,
automatic view membership for newly attached Parts, independent or
linked-artwork duplication of Parts and Peg subtrees, branch deletion with
view cleanup, Part detachment and Peg dissolution that preserve supported rest
poses. View sets can be reordered and navigated, updated for one selected
Part, or applied over an explicit half-open frame range. All document edits
are undoable Session transactions. A 19-part original-art scene now exercises
typed roles, coordinated view switching, independent character duplication,
save/reopen and render equivalence. HM-03 still needs artist validation and
animated-ancestor behavior remains deliberately unsupported.
