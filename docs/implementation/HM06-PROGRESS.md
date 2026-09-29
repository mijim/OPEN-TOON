# HM-06 animated deformation progress

HM-06 is in progress on macOS 15.5 arm64. The current format-11 subset animates
one two-segment bone chain or one cubic curve per bound Part substitution and
links a child Part to its parent's evaluated bone tip. It
does not yet satisfy the full HM-06 interaction and artistic quality gate.

The [twenty-second visual shot study](HM06-VISUAL-SHOT.md) now exercises the
continuous rig with camera, face/hand/mouth substitutions and a torso curve
over the agreed 480-frame timing. Audio, mattes and published controls remain
in their owning slices; the study does not claim the complete Harmony Moment.
The [Milo continuous rig study](HM06-MILO.md) is now the character for new
animation work. Its user-provided SVG/PNG source remains intact; the saved
17-Part example uses four one-image limbs, elbow/knee bones and linked hands
and ankles. Rest, bent, return, native save/reopen and full-frame connectivity
are checked. The earlier Clockwork fixtures remain regression coverage.
The saved 48- and 480-frame studies now use alpha-following contour meshes
for each continuous arm and leg. The same editable middle elbow/knee bones,
keys, sleeve variants and attached hands/feet remain. A sharper 40 px elbow
transition reduces the inflated inner-joint contour at frame 24 and at 90° while
retaining a connected silhouette. The [current frame-24 bend](hm06-contour-bend-24.png)
and [90° stress pose](hm06-contour-bend-90.png) were inspected. This is a
bounded visual improvement; owner artistic approval remains open.
The 480-frame preview/connectivity sweep also found and corrected a visible
angle mismatch at the alternate-sleeve change. **Match previous pose** now
records that incoming bone or curve key explicitly at a compatible drawing
boundary; the visual shot uses it before the animation continues. The action
is undoable and tested through native save/reopen.

The current original-art revision adds a muted cap to both head views and
2 × 2 edge coverage to the registered PNG drawings. The continuous limbs,
their middle elbow/knee bones and the 19-role intake remain in place. The
independent reference compositor can differ by at most two 8-bit channel
levels at antialiased edges from Qt's premultiplied compositor; the tests bound
both that maximum and the number of changed pixels. The generated art, three
editable projects and displayed stills were regenerated together. This is
candidate visual direction rather than owner artistic acceptance.

## Working subset

- The selected substitution retains its rest mesh and UVs. Bone joints,
  a saved elbow transition and normalized per-vertex distal weights produce
  connected forward-kinematic poses. Influence rotates around the posed elbow
  to preserve distance at intermediate weights. Curve controls retain a fixed rest
  parameter per vertex and move the mesh by the posed-minus-rest cubic field.
  The evaluated bone joints are now exposed by the domain evaluator and used
  by the canvas controls and the bounded child-Part attachment evaluator. The
  continuous-limb fixture now follows hand/foot substitutions with saved links,
  without duplicating endpoint motion keys. Broader attachments, IK and portable
  rig templates remain HM-09 work.
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
- Format 9 persists controls, weights and keys; format 10 introduced one
  parent bone-tip link per child Part. Format 11 saves the link's rest tip and
  distal axis at attachment time, so changing the source's initial view cannot
  redefine the anchor. The first save of an older project preserves its
  source-version backup, including `.pre-v10.bak` for format 10.
  The link requires a direct parent Part with a bound two-segment bone on every
  exposed source drawing and rest at frame zero. Parent selection and a Rig
  menu action bind or unbind it through undoable document commands.
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
  frame-zero scene agrees with `reference_0000.png` within the bounded
  antialias-compositing tolerance; an animated arm
  and torso reopen to identical frame-12 pixels. A manually placed elbow,
  deforming upper sleeve, and rigidly pivoted forearm/hand produce a 70°
  [extreme pose](hm06-bone-extreme.png) with unchanged rest pixels and identical
  reopened output. The earlier double-sleeve elbow bulge is absent in this
  inspected pose; broader joint and artwork review remains open. The owner's
  continuity correction is exercised by an additional
  [15-artwork-Part rig](hm06-continuous-limbs.png): each complete arm and leg
  is one image and one bound mesh, with an elbow or knee as its middle joint.
  The test bends all four limbs, follows separate hand/foot substitutions with
  four saved links, and checks unchanged rest pixels and identical reopened
  frame-24 output. Per-frame joint coordinates match the follower's world
  position. Detach/attach undo and redo, SQLite save/reopen, compatible source
  substitution switching and atomic rejection of source bone removal or an
  unbound substitution also pass.
  A retargeted alternate sleeve with a displaced and rotated rest wrist now
  places the linked hand on the active evaluated tip, including its distal
  direction. The previous per-variant rest anchor missed that tip by 14.42 px
  in the regression case. The corrected mapping passes command undo/redo,
  serialization and SQLite reopen. Format 11 additionally preserves the
  attachment-time anchor when an alternate source drawing is exposed at frame
  zero; the format-10 migration and readable backup are covered by a real
  linked-project fixture.
  An [openable 48-frame rig study](../../examples/clockwork-continuous.otoon)
  combines the continuous limbs and links with a changed sleeve binding, fist
  substitution and coordinated face view. A saved Front view returns at frame
  40. The [frame-36 capture](hm06-continuous-pose-switch.png), frame-44 render,
  undo/redo and reopened project are covered by the native image test; the
  saved example is compared with the constructed test document. The Qt Quick
  smoke test opens that exact project, selects its linked hand, changes frames
  across the view switch and presents the canvas.
  A source-art correction removed a stray point behind the ear in the
  three-quarter head drawing; the example and affected reference stills were
  regenerated without changing their registration or substitution identities.
  Retuning one posed arm's radius from 65 to 55 pixels preserves rest pixels,
  changes the bend, keeps the assembled silhouette connected and reopens to
  identical output.
  A deterministic asset test checks that each source limb is one connected
  alpha silhouette and covers all three registered joints. The render test
  also checks one connected character silhouette at rest and in the bent pose.
  A separate [90° stress pose](hm06-continuous-limbs-90.png) bends all four
  continuous limbs around their middle joints. The assembled silhouette stays
  connected at every integer frame from rest through frame 36, with no render
  failure; rest pixels and reopened frame-36 pixels are identical. The source
  redraw removes elbow and knee crease strokes that made one-piece limbs read
  like separate artwork. The 65 px transition accepts this 90° pose; reducing
  it to 55 px folds the mesh and rejects the new key without changing the
  document on the regular rectangular grid. An explicit **Contour** bind now
  fits each mesh row to the image alpha with a one-cell lookahead and one-pixel
  margin. The [contour 90° stress pose](hm06-contour-bend-90.png) bends all four
  one-piece limbs at a sharper 40 px transition without a folded triangle;
  a 35 px transition on the left arm rejects atomically. Rest rendering is
  byte-identical; a source-alpha coverage check protects every opaque texel,
  the posed character remains connected, and serialized and reopened frame-36
  pixels match. The native editor smoke also binds, undoes and redoes the
  contour profile. The continuous candidate now uses an original joined
  trouser-waist drawing over both leg roots; its long highlights flow into
  the thigh art without the former pouch-shaped pelvis or a lower cap line.
  The regenerated example and rest, view-switch and 90° captures are checked
  against the saved document. The updated 15-Part candidate passed 128/128
  CTest entries, the deterministic art test and native smoke; three 40-sample
  native drags measured p95 16.94–31.11 ms and peak resident memory below
  279 MB on the same M1 Pro. A subsequent sleeve redraw replaced the pointed
  shoulder caps and straight upper-arm edges with continuous cubic contours;
  the registered joints, single-image limbs and saved binding structure are
  unchanged. The updated rest and 90° images show a softer outer shoulder
  silhouette. The updated candidate passed 128/128 CTest entries and native
  smoke; three 40-sample drags measured p95 16.69–17.97 ms and peak resident
  memory below 280 MB. This is a deformation stress image, not a polished animation
  pose; the hood-to-sleeve seam and knee volume still need artistic refinement.
  The candidate has no elbow or knee image seam; owner visual approval is still
  open.
  The later leg redraw replaces the two angular trouser silhouettes with
  continuous cubic contours, a modestly fuller knee and a narrower ankle.
  The first wider knee candidate folded the right contour mesh at 90° and
  was rejected; the final contour passes the same 40 px transition and
  rejects the unsafe 35 px case. Registered hip/knee/ankle joints and the
  one-image, one-mesh limb ownership are unchanged. Both openable examples
  and their representative captures were regenerated. The local macOS suite
  passes 166/166 entries, the deterministic asset test and native HM-07
  dashboard smoke (40 mouse-to-present samples, p95 18.02 ms). This is a
  smaller silhouette correction; full artistic approval remains open.
- The native `--smoke-test` drags a bone tip and curve tangent, checks
  temporary preview isolation, cancellation, undo/redo, bone and curve rest-control
  retarget, a 6 × 16 mesh grid, numeric and on-canvas elbow influence tuning,
  cancelled preview/undo/redo, Rest key, same-Part
  timeline key paste/move and project reopen on
  a checker Part. A second checker Part links through the editor, undoes,
  redoes and reopens with its parent bone-tip link. A mirrored, rotated and zoomed control drag also round-trips
  through undo. The inspected [original-art detail](hm06-bone-curve-detail.png)
  shows the bounded arm/torso deformation after the rotation correction.
  Additional elbow shapes and larger bends still need artistic acceptance.
- The macOS optimized build passes 128/128 CTest entries, the continuous-limb
  Python fixture test and native smoke. A 40-frame native drag measurement on
  the 15-artwork-Part continuous rig gave p95 input-to-`frameSwapped` 17.26 ms
  and peak process resident memory 274 MB on Apple M1 Pro before attachments.
  A fresh 40-sample native run of the format-10 linked rig measured p95
  17.18 ms and peak process resident memory 275 MB. After the variant-anchor
  correction, three native runs measured p95 17.86–18.15 ms and peak resident
  memory below 279 MB. With the anchor saved in format 11, three runs measured
  p95 18.00–18.45 ms and peak resident memory below 280 MB. The earlier
  measurements were single runs;
  repeatability and the complete B4 shot budget remain open.
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
  A [20-second deformation workload](HM06-INTERACTION.md#twenty-second-deformation-workload-2026-09-29)
  retimes the linked character to 480 frames, checks all preview frames and
  selected full-resolution/reopened frames, and measures three native drag
  runs. Its p95 was 17.12–18.28 ms and peak resident memory stayed below
  278 MB; one 480-frame headless 1080p run averaged 5.07 ms/frame. This
  synthetic timing workload does not yet include audio, mattes, animated
  camera or published controls.
  The later [visual shot study](HM06-VISUAL-SHOT.md) adds the animated camera,
  torso curve and timed face/hand/mouth choices over all 480 frames. It passes
  131/131 CTest entries and native open/scrub/save/reopen smoke, including an
  explicit sleeve-pose match with undo/redo. At frames 12 and 360, six native
  drag runs measured p95 16.93–21.75 ms and peak resident memory below 286 MB.
  One 480-frame 1080p renderer run averaged 5.45 ms/frame. This remains a
  visual HM-06 workload, with audio, mattes and published controls pending
  their owning slices.

## Remaining acceptance

Measure the complete B4 shot against the HM proposed budgets; inspect extreme
bends, influence tuning, joint seams and texture behavior on the reference
shot. Qualify additional poses and source
profiles, native interaction under zoom/rotation and an artist workflow. The
current two-segment/cubic profile does not provide envelope, IK, deformer
stacking or arbitrary shape-aware weights. P09 and full DEF catalog features
remain open.
