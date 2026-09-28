# HM-06 animated deformation progress

HM-06 is in progress on macOS 15.5 arm64. The current format-9 subset animates
one two-segment bone chain or one cubic curve per bound Part substitution. It
does not yet satisfy the full HM-06 interaction and artistic quality gate.

## Working subset

- The selected substitution retains its rest mesh and UVs. Bone joints,
  a saved elbow transition and normalized per-vertex distal weights produce
  connected forward-kinematic poses. Influence rotates around the posed elbow
  to preserve distance at intermediate weights. Curve controls retain a fixed rest
  parameter per vertex and move the mesh by the posed-minus-rest cubic field.
- Both profiles save frame keys, sample linear/held/smooth interpolation and
  evaluate through the same preview and output renderer. A later first key
  anchors rest at frame zero. "Rest key" records rest at the current frame
  while preserving other keys. Deformer edits, binding and removal use the
  document command/undo path; invalid input rejects before publication.
- Properties offers Bone chain and Curve on a selected bound Part. The Mesh
  tool drags joints or curve controls directly on canvas with a cancellable
  temporary preview. Its grid now follows the evaluated pose during animation
  and preview. Static vertex editing is unavailable while a deformer
  is attached. Switching substitutions resolves the matching chain.
- Format 9 persists controls, weights and keys. The first save of format 8
  preserves a source-version `.pre-v8.bak` before upgrading.
- Insert/remove frames shift or remove deformer keys with the scene clock.
  Clear removes them from the selected Part/range and remains atomic with
  exposures and layer pose keys. Same-Part range paste/move/stretch transfers
  each substitution's keyed poses and rejects retiming collisions. Cross-scene
  and different-Part deformer-key pastes reject before mutation until rig
  binding transfer is available. Independent drawing paste of a bound Part
  also rejects to avoid silently dropping its mesh.

## Verification

- `tests/deformer_tests.cpp` covers connected joints, interpolation, rest,
  undo, invalid geometry/weights, rejected folds, serialization and range
  transfer with exact evaluated endpoints.
  `tests/mesh_render_tests.cpp` covers saved preview/Display/Write pixels,
  separate substitution chains, and the original 19-part scene. The assembled
  frame-zero scene is byte-identical to `reference_0000.png`; an animated arm
  and torso reopen to identical frame-12 pixels. A manually coordinated 70°
  pose of upper arm, lower arm and hand keeps rest pixels unchanged and
  reopens identically. The [extreme pose](hm06-bone-extreme.png) still shows an
  elbow bulge that blocks artistic acceptance.
- The native `--smoke-test` drags a bone tip and curve tangent, checks
  temporary preview isolation, cancellation, undo/redo, Rest key, same-Part
  timeline key paste/move and project reopen on a checker Part. A
  mirrored/rotated/zoomed control drag also
  round-trips through undo. The inspected [original-art detail](hm06-bone-curve-detail.png)
  shows the bounded arm/torso deformation after the rotation correction. The
  elbow overlap and larger bends still need correction and artistic acceptance.
- The macOS optimized build passes 121/121 CTest entries and native smoke.
  On Apple M1 Pro, three earlier local 1920×1080 renders of the assembled
  19-part scene with one bone and one curve averaged 3.25 ms/frame. After the
  fixture's toon-art revision, four runs averaged 3.30 ms/frame (3.04–3.56).
  This is
  renderer cost, separate from native input-to-present latency and full-shot
  memory. No new dependency or license was added; Eigen remains a candidate.
- The [native interaction measurement](HM06-INTERACTION.md) on the same M1 Pro
  used 40 input-to-`frameSwapped` samples in each of four runs of the 19-part
  subset with the revised toon artwork. p95 was 15.71–17.91 ms and process
  peak resident memory was below 273 MB. The current subset meets the proposed 50 ms/2 GiB limits on this
  host; the complete B4 shot remains unmeasured.

## Remaining acceptance

Measure the complete B4 shot against the HM proposed budgets; improve and
inspect extreme bends, influence tuning, joint seams and
texture behavior on the reference shot. Qualify additional poses and source
profiles, native interaction under zoom/rotation and an artist workflow. The
current two-segment/cubic profile does not provide envelope, IK, deformer
stacking or arbitrary shape-aware weights. P09 and full DEF catalog features
remain open.
