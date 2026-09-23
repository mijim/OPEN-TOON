# Next implementation tasks — character-first

HM-00 reference assets, foundation contracts and owner artistic review are
accepted. The bounded HM-02 artwork-intake contract is complete; HM-01 typed
property work is accepted; HM-03 rigid character work is underway. Use
[the canonical contract DAG](HARMONY-MOMENT.md), not a
phase number, to determine readiness. Two active slices maximum if owners exist;
with one implementer use the recommended order below. Every delivered block must
undo, save/reopen and exercise its real failure boundaries before consumers start.

## Delivered contracts and immediate tickets

| Order / ID | Concrete next task | Prerequisite | Observable acceptance | Parallel option |
|---|---|---|---|---|
| Done / HM-00-A | Create the original/PNG character asset fixture and scripted shot rubric; record baseline host, scene size, frame/latency/memory/audio targets and asset provenance | Inspect current code/status | Reproducible 20-second shot spec and ten-minute sync fixture; no proprietary artwork | HM-00-B contract review within the same slice |
| Done / HM-00-B | Specify typed part/peg/property identity, rest versus authored pose, driver ownership, space/time/color boundaries and format-3 compatibility | Current structs/renderer/storage audit | Reviewed schema examples and migration/failure acceptance; restrict the renderer spike to a real consumer | Fixture preparation above |
| Done / HM-01-A | Expose typed property addresses through existing Session/evaluation adapters; preserve full-pose keys and introduce new track payloads only for real consumers | Accepted HM-00 | Old-scene evaluated poses/easing preserved; dangling/duplicate/type-invalid references rejected; undo and migration backup tests | HM-02 after HM-00 with a separate owner |
| Done / HM-02-A | Qualify transparent PNG registration, then atomic parts/sequence intake with explicit mode, order/gap report and cancellation | Accepted HM-00 contracts | Original artwork and imported parts round-trip; failed batch leaves no partial scene | HM-01 |
| Partial / HM-03-A | Add character root/part roles and separate peg hierarchy; preserve-world reparenting and permanent pivot editing | HM-01 + HM-02 | Assemble/rest-pose a rigid character without registration jumps; reject unsupported shear/singular cases | HM-10 audio is independently eligible |
| Partial / HM-03-B | Thumbnail substitutions, held selectors, coordinated views, 19-part save/reopen fixture and identity-safe rig-branch edits are implemented | HM-03-A | Obtain artist review of 19-part assembly/view interaction; qualify animated-ancestor or explicitly bounded reparenting and address any observed workflow blockers before accepting the contract | HM-04 and HM-13 are accepted; no deformers yet |
| Done / HM-04-A | Typed graph, Display/Write, alpha matte, opt-in linear-sRGB profile, bounded revision cache, hierarchy-aware invalidation and cancellation. Speculative next-frame publication, native playback/scrub, color charts and animated original-art cost evidence close the bounded contract | HM-01 | [Acceptance record](../implementation/HM04-ACCEPTANCE.md); P10 remains open | HM-13 is complete; HM-10 is eligible |
| Done / HM-13-A | One animated orthographic output camera with direct frame handles, guides, format-7 persistence and preview/export parity | HM-04 | [Acceptance record](../implementation/HM13-ACCEPTANCE.md); P06 remains open | HM-10 audio is independently eligible |
| 8 / HM-05-A | Prove saved rest mesh/UV/weights + actual texture-warp rendering on bone/curve checker fixtures | HM-03 + HM-04 | Measured joint/texture quality and bounded cost; accepted backend/library decision or documented blocker | HM-07 controls or HM-10 audio |

HM-01-A has accepted typed persistence, source-version migration and failure evidence.
HM-03-A and HM-03-B have working subsets; complete the full rigid-character
acceptance journey and its failure cases before accepting HM-03. Later payload
schemas are introduced in their owning slices, with the common migration rules.

HM-00 evidence: [original generated fixture](../../tests/fixtures/harmony-moment/README.md)
and [foundation contract](../architecture/adr/023-harmony-foundation-contracts.md)
are accepted with [owner artistic review](../implementation/HM00-REVIEW.md).
A rigid, saved/reopened 19-part baseline matches the first reference still.
Producing the complete animated shot belongs to HM-15; it does not gate HM-01.

## Continue in dependency order

| Recommended priority | Work | Hard join | Parallel work once its prerequisites pass |
|---|---|---|---|
| 9 | HM-06 bone/curve authoring, animation and variant binding | HM-05 | HM-07 poses/declarative controls; HM-10 audio |
| 10 | HM-07 character poses, widgets, one-dimensional sliders and Animator/Rig views | HM-03 + HM-04 | HM-06; no scripting needed |
| 11 | HM-08 Quick Rig FK recipe: assign roles, place guides, preview/correct/commit | HM-03 + HM-07 | HM-06 or HM-10 |
| 12 | HM-09 attachments, limited two-bone IK, optional rig recipes and dependency-closed templates | HM-06 + HM-07 + HM-08 | HM-10/11 and HM-12/13 branches |
| Parallel branch A | HM-10 WAV device clock/waveform/trim/mix/scrub, then HM-11 mouth mapping and manual/timing-file lipsync | HM-01; HM-11 additionally needs HM-03 | Character/deformer work; audio need not wait for rig completion |
| Parallel branch B | HM-12 node editor, groups, cutters, ordered overlap and template graph closure | HM-03 + HM-04 | Deformers and audio; no solver prerequisite |
| Complete branch C | HM-13 one output camera with pan/zoom/rotation and framing | HM-04 | HM-12 or HM-10; no multiplane prerequisite |
| 13 | HM-14 revision-safe cached preview and exact PNG/WAV/manifest delivery | HM-10 + HM-12 + HM-13 | Final HM-09 rig/template integration |
| 14 | HM-15 second-animator complete shot, second-scene reuse, save/reopen/export equivalence and fault recovery | HM-09 + HM-11 + HM-14 | Qualification tasks only; no shortcut around an unfinished branch |

Branches describe technical independence, not three additional active tasks. With a
small team prioritize the binding/deformation risk path, then fill another slot with
an independent audio/control task. Select the next ready task using `roadmap.py next`.
The estimated longest chain currently runs HM-00 → HM-01 → HM-03 → HM-05 → HM-06 →
HM-09 → HM-15; HM-04 and all other incoming contracts must also pass at each join.

## Follow-on queue, not milestone blockers

After the workflow gate, use artist evidence to choose the next bounded task:
review-movie export (FFmpeg adapter/profile after exact PNG/WAV timing), layered
PSD/SVG intake (parser and loss reporting), automatic lipsync proposals (after manual
mapping/correction), envelopes (after rest/render/binding), pose grids (after masked
poses and interpolation), pins/constraint keys (after bounded IK and solve-order
validation), and multiplane (after output-camera contract). Preserve all remaining
P00–P22 scope and primary completion owners.

Original BOOT/BASE/FILM IDs remain in [FOUNDATION-BACKLOG.md](FOUNDATION-BACKLOG.md).
Take an outstanding task from that backlog only when it supplies a named consumer
contract or a later full-phase exit criterion; do not redo existing work blindly.

## Ticket contract

```yaml
id: HM-06-A
status: planned
slice: HM-06
phase: P09
feature_ids: [DEF-001, DEF-003, DEF-008, DEF-010]
owner_module: deformation
requires: [HM-05]
supported_subset: Bone/curve chains using the accepted rest-mesh and render profile.
contract_changes: [versioned_bind_payload, animated_deformer_properties]
acceptance:
  - Bend and reset the fixture without changing the rest source.
  - Switch compatible drawings and preserve their binding references.
  - Preview, undo, reopen and export produce the same evaluated pose.
evidence: []
known_limits: [no_envelope_or_shape_aware_solver]
```

Pin new library versions only when the consumer spike justifies adoption. Run relevant
tests after coherent implementation blocks; this documentation change requires no
application rebuild or new Linux/Windows compilation.
