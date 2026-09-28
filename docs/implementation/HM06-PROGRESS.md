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
  and preview. Rest controls mode moves bone root, elbow or tip, or any cubic
  control, in the saved setup and validates existing poses before atomic
  publication. Bone moves recalculate weights; curve moves preserve keyed
  offsets from rest and recalculate vertex parameters. Static vertex editing
  is unavailable while a deformer is attached. Switching substitutions
  resolves the matching chain.
- Properties now lets the animator choose a 1–32-cell mesh grid per axis, so
  the 6 × 16 continuous-limb topology used by the render fixture is buildable
  from the app. The saved grid dimensions are shown after binding. The elbow
  influence radius is editable in pixels or by dragging its on-canvas rest
  handle. The dashed circle previews the influenced area. Changes recalculate
  weights, keep authored angle keys/rest pixels and reject folded existing
  poses atomically.
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
  undo, influence-radius retargeting, invalid geometry/weights, rejected folds,
  serialization and range transfer with exact evaluated endpoints.
  `tests/mesh_render_tests.cpp` covers saved preview/Display/Write pixels,
  separate substitution chains, and the original 19-part scene. The assembled
  frame-zero scene is byte-identical to `reference_0000.png`; an animated arm
  and torso reopen to identical frame-12 pixels. A manually placed elbow,
  deforming upper sleeve, and rigidly pivoted forearm/hand produce a 70°
  [extreme pose](hm06-bone-extreme.png) with unchanged rest pixels and identical
  reopened output. The earlier double-sleeve elbow bulge is absent in this
  inspected pose; broader joint and artwork review remains open. The owner's
  continuity correction is exercised by an additional
  [15-artwork-Part rig](hm06-continuous-limbs.png): each complete arm and leg
  is one image and one bound mesh, with an elbow or knee as its middle joint.
  The test bends all four limbs, follows separate hand/foot substitutions,
  and checks unchanged rest pixels and identical reopened frame-24 output.
  Retuning one posed arm's radius from 65 to 55 pixels preserves rest pixels,
  changes the bend, keeps the assembled silhouette connected and reopens to
  identical output.
  A deterministic asset test checks that each source limb is one connected
  alpha silhouette and covers all three registered joints. The render test
  also checks one connected character silhouette at rest and in the bent pose.
  This candidate
  has no elbow or knee image seam; owner visual approval is still open.
- The native `--smoke-test` drags a bone tip and curve tangent, checks
  temporary preview isolation, cancellation, undo/redo, bone and curve rest-control
  retarget, a 6 × 16 mesh grid, numeric and on-canvas elbow influence tuning,
  cancelled preview/undo/redo, Rest key, same-Part
  timeline key paste/move and project reopen on
  a checker Part. A mirrored, rotated and zoomed control drag also round-trips
  through undo. The inspected [original-art detail](hm06-bone-curve-detail.png)
  shows the bounded arm/torso deformation after the rotation correction.
  Additional elbow shapes and larger bends still need artistic acceptance.
- The macOS optimized build passes 125/125 CTest entries, the continuous-limb
  Python fixture test and native smoke. A 40-frame native drag measurement on
  the 15-artwork-Part continuous rig gave p95 input-to-`frameSwapped` 17.26 ms
  and peak process resident memory 274 MB on Apple M1 Pro (one run; broader
  repeatability and full-shot budget remain open).
  On Apple M1 Pro, three earlier local 1920×1080 renders of the assembled
  19-part scene with one bone and one curve averaged 3.25 ms/frame. After the
  latest independent toon redraw, four runs averaged 3.18 ms/frame
  (3.09–3.26). This is renderer cost, separate from native input-to-present
  latency and full-shot memory. No new dependency or license was added; Eigen
  remains a candidate.
- The [native interaction measurement](HM06-INTERACTION.md) on the same M1 Pro
  used 40 input-to-`frameSwapped` samples in each of four runs of the 19-part
  subset with the latest redraw. p95 was 14.90–16.76 ms and process
  peak resident memory was below 273 MB. The current subset meets the proposed
  50 ms/2 GiB limits on this host; the complete B4 shot remains unmeasured.

## Remaining acceptance

Measure the complete B4 shot against the HM proposed budgets; inspect extreme
bends, influence tuning, joint seams and texture behavior on the reference
shot. Qualify additional poses and source
profiles, native interaction under zoom/rotation and an artist workflow. The
current two-segment/cubic profile does not provide envelope, IK, deformer
stacking or arbitrary shape-aware weights. P09 and full DEF catalog features
remain open.
