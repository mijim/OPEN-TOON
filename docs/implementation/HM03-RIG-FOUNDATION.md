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

This supports a simple rigid cut-out workflow using the existing timeline and
Properties panel. It does not complete HM-03: thumbnail browsing, coordinated
view sets, portable character duplication/instance remapping and a full
19-part saved character acceptance fixture remain. The current reparent tool
cannot solve animated-parent motion, reparent an animated child or store shear.
Rest pivot placement must occur before that layer is animated. Subsequent HM-03 work should
make substitutions visually scannable and add atomic multi-part view sets
before HM-05 or HM-07 consumes this rig.
