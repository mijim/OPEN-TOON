# Next implementation tasks — character-first

**Current execution guide.** Begin at [NOW](NOW.md), read [implementation status](../implementation/STATUS.md), run `python3 scripts/roadmap.py next`, then inspect `python3 scripts/roadmap.py slice HM-06` (or the chosen eligible slice). Review its scope, feature requirements and acceptance, and verify every `requires` contract has accepted evidence before starting. `planned` does not mean ready; an unaccepted dependency blocks its consumer. An accepted bounded contract does not complete its owning phase. Keep at most two bounded slices active with separate owners; with one implementer, follow the recommended priority below.

HM-00 reference assets, foundation contracts and owner artistic review are
accepted. The bounded HM-02 artwork-intake contract is complete; HM-01 typed
property work and bounded HM-03 rigid character work are accepted. Use
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
| Done / HM-03-A | Character root/part roles, peg hierarchy, preserve-world reparenting and permanent rest pivots | HM-01 + HM-02 | [Bounded acceptance](../implementation/HM03-ACCEPTANCE.md); supported rest and shared-ancestor motion preserve pixels | HM-10 audio is independently eligible |
| Done / HM-03-B | Thumbnail substitutions, held selectors, coordinated views, 19-part save/reopen fixture, identity-safe rig-branch edits and bounded shared-ancestor animated reparenting | HM-03-A | Owner-delegated 19-part import/assembly/view journey, reference pixels, undo/reopen and explicit failure boundaries pass; independent HM-15 animator review remains | HM-04 and HM-13 are accepted; no deformers yet |
| Done / HM-04-A | Typed graph, Display/Write, alpha matte, opt-in linear-sRGB profile, bounded revision cache, hierarchy-aware invalidation and cancellation. Speculative next-frame publication, native playback/scrub, color charts and animated original-art cost evidence close the bounded contract | HM-01 | [Acceptance record](../implementation/HM04-ACCEPTANCE.md); P10 remains open | HM-13 is complete; HM-10 is eligible |
| Done / HM-13-A | One animated orthographic output camera with direct frame handles, guides, format-7 persistence and preview/export parity | HM-04 | [Acceptance record](../implementation/HM13-ACCEPTANCE.md); P06 remains open | HM-10 audio is independently eligible |
| Done / HM-05-A | Prove saved rest mesh/UV + actual texture-warp rendering on checker and original character fixtures | HM-03 + HM-04 | [Bounded acceptance](../implementation/HM05-ACCEPTANCE.md): mouse bind/rest/pose, exact rest pixels, vector proxy, format-8 migration and measured 19-part render | HM-07 controls or HM-10 audio |

HM-01-A has accepted typed persistence, source-version migration and failure evidence.
HM-03-A and HM-03-B have accepted bounded rigid-character evidence. Later payload
schemas are introduced in their owning slices, with the common migration rules.

HM-00 evidence: [original generated fixture](../../tests/fixtures/harmony-moment/README.md)
and [foundation contract](../architecture/adr/023-harmony-foundation-contracts.md)
are accepted with [owner artistic review](../implementation/HM00-REVIEW.md).
A rigid, saved/reopened 19-part baseline matches the first reference still.
Producing the complete animated shot belongs to HM-15; it does not gate HM-01.

## Continue in dependency order

| Recommended priority | Work | Hard join | Parallel work once its prerequisites pass |
|---|---|---|---|
| 9 / parked | HM-06 bone/curve authoring, animation and variant binding; finish quality and interaction gates | HM-05 accepted | HM-12 composition is active |
| 10 / parked | HM-07 character poses, widgets, one-dimensional sliders and Animator/Rig views | HM-03 + HM-04 | HM-12 composition is active |
| 11 | HM-08 Quick Rig FK recipe: assign roles, place guides, preview/correct/commit | HM-03 + HM-07 | HM-06 or HM-10 |
| 12 | HM-09 attachments, limited two-bone IK, optional rig recipes and dependency-closed templates | HM-06 + HM-07 + HM-08 | HM-10/11 and HM-12/13 branches |
| Parallel branch A / parked | HM-10 PCM16 import, exact waveform, clip edits, repeats, source-sample fades, miniaudio preview, scrub, band-limited rate conversion and selected-range WAV export pass as a subset; qualify hardware presentation and rate quality before HM-11 mouth mapping | HM-01; HM-11 additionally needs HM-03 | Character/deformer and audio work is parked while one implementer advances composition |
| Parallel branch B / active | HM-12 format-19 inside/outside cutters and derived Nodes workspace with animated Opacity nodes and clicked previews, format-20 persistent bypass and format-21 visible cutter sources, format-24 opacity bypass and format-25 Normal/Multiply/Screen blending pass as a subset; general editable graph, groups, overlap recipes and template closure remain | HM-03 + HM-04 | Deformers and audio; no solver prerequisite |
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
status: in_progress
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
evidence: [docs/implementation/HM06-PROGRESS.md]
known_limits: [extreme_bend_and_interaction_budget_pending, no_envelope_or_shape_aware_solver]
```

Pin new library versions only when the consumer spike justifies adoption. Run relevant
tests after coherent implementation blocks; this documentation change requires no
application rebuild or new Linux/Windows compilation.
