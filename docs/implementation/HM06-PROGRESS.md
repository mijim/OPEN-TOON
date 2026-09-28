# HM-06 animated deformation progress

HM-06 is in progress on macOS 15.5 arm64. The current format-9 subset animates
one two-segment bone chain or one cubic curve per bound Part substitution. It
does not yet satisfy the full HM-06 interaction and artistic quality gate.

## Working subset

- The selected substitution retains its rest mesh and UVs. Bone joints,
  a saved elbow transition and normalized per-vertex distal weights produce
  connected forward-kinematic poses. Curve controls retain a fixed rest
  parameter per vertex and move the mesh by the posed-minus-rest cubic field.
- Both profiles save frame keys, sample linear/held/smooth interpolation and
  evaluate through the same preview and output renderer. A later first key
  anchors rest at frame zero. "Rest key" records rest at the current frame
  while preserving other keys. Deformer edits, binding and removal use the
  document command/undo path; invalid input rejects before publication.
- Properties offers Bone chain and Curve on a selected bound Part. The Mesh
  tool drags joints or curve controls directly on canvas with a cancellable
  temporary preview. Static vertex editing is unavailable while a deformer
  is attached. Switching substitutions resolves the matching chain.
- Format 9 persists controls, weights and keys. The first save of format 8
  preserves a source-version `.pre-v8.bak` before upgrading.
- Insert/remove frames shift or remove deformer keys with the scene clock.
  Clear removes them from the selected Part/range and remains atomic with
  exposures and layer pose keys. Range paste/move/stretch with deformer keys
  explicitly rejects until those clipboard operations can transfer the full
  binding safely.

## Verification

- `tests/deformer_tests.cpp` covers connected joints, interpolation, rest,
  undo, invalid geometry/weights, rejected folds and serialization.
  `tests/mesh_render_tests.cpp` covers saved preview/Display/Write pixels,
  separate substitution chains, and the original 19-part scene. The assembled
  frame-zero scene is byte-identical to `reference_0000.png`; an animated arm
  and torso reopen to identical frame-12 pixels.
- The native `--smoke-test` drags a bone tip and curve tangent, checks
  temporary preview isolation, cancellation, undo/redo, Rest key and project
  reopen on a checker Part. A mirrored/rotated/zoomed control drag also
  round-trips through undo. The inspected [original-art detail](hm06-bone-curve-detail.png)
  shows the bounded arm/torso deformation. It reveals a noticeable taper at
  the elbow; larger bends and seam behavior still need correction/acceptance.
- The macOS optimized build passes 121/121 CTest entries and native smoke.
  On Apple M1 Pro, three local 1920×1080 renders of the assembled 19-part
  scene with one bone and one curve averaged 3.25 ms/frame. This is renderer
  cost, not measured control-to-preview p95 or full shot memory. No new
  dependency or license was added; Eigen remains a candidate.

## Remaining acceptance

Measure control-to-preview p95 and resident memory against the HM proposed
budgets; improve and inspect extreme bends, influence tuning, joint seams and
texture behavior on the reference shot. Qualify additional poses and source
profiles, native interaction under zoom/rotation and an artist workflow. The
current two-segment/cubic profile does not provide envelope, IK, deformer
stacking or arbitrary shape-aware weights. P09 and full DEF catalog features
remain open.
