# HM-03 bounded rigid-character acceptance

The owner delegated the 19-part workflow review to the implementer on
2026-09-28. On the macOS arm64 development profile, the bounded HM-03 rigid
character contract is accepted. This is not an independent second-animator
review, a completed P08 phase, a deformation claim or a supported 1.0 release.

The user-facing controller journey in `tests/export_tests.cpp` imports all 19
original registered PNG parts, creates a Character, attaches and names each
Part, sets its rest placement and paints in the intended order. Its frame-zero
render matches every pixel of `reference_0000.png`. The same journey adds a
Peg over the right arm, reparents the hand without a jump, and keeps its two
transform keys unchanged while switching coordinated views. It imports actual
three-quarter head, hair, eyes and mouth images as named substitutions, adds a
pointing-hand substitution, returns to the front view and then reapplies the
three-quarter view. Rendered front and side poses were visually inspected;
their saved/reopened pixels, undo/redo and independently duplicated character
IDs agree with the tested document state. The inspected images are
[`hm03-front.png`](hm03-front.png) and
[`hm03-three-quarter.png`](hm03-three-quarter.png).

`tests/rigging_tests.cpp` also renders every frame of a 48-frame character-root
move before and after a reparent inside its shared moving branch. It checks
mirrored nonuniform scale preservation, and atomic rejection of singular,
sheared, differently animated and opacity-impossible cases. Existing view
tests reject missing members before changing any part. The native
`--smoke-test` passed on macOS for QML canvas input, timeline and animation
actions, though it does not itself click every Rig menu item. The full
102-entry CTest suite and `scripts/validate_docs.py` passed after this block.

The workflow still requires an artist to choose correct paint order and rest
placement. Reparenting between differently animated branches, animated child
reparenting and shear storage are unsupported and reported. Quick Rig, poses,
deformation, portable templates and the separate second-animator HM-15 journey
remain open. This acceptance covers only HM-03's rigid-character contract.
