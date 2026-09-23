# Character-first roadmap reorientation

**Reference — decision rationale snapshot, not the active execution queue.** For current work, start at [NOW](NOW.md).

Decision date: 2026-09-21. Baseline: source `9dcac48` / application
`0.2.0-experimental.10`. This revision changes planning and its validation tools only.
Application implementation, dependency locks, file format and completion evidence are unchanged.

## Goal and bounded outcome

Prioritize a **Harmony Moment**: a reusable speaking character shot that an animator
can actually finish. Drawing or registered PNG import → character/cut-out → substitutions
→ rigging → bone/curve deformers → published controls/poses → animation with dialogue
and editable lip sync → node masks/composition → camera → preview/save/reopen/export.

The [canonical delivery plan](HARMONY-MOMENT.md) defines a 20-second shot, exact feature
subsets, dependency contracts and a final artist/reliability gate. It precedes full
P11/1.0; it does not rename the current prototype into a professional product. The
required output is a synchronized PNG sequence, PCM WAV and timing/color manifest.
Review-movie encoding is the first media follow-on, explicitly outside this first gate.

The product flow above is not the engineering sequence: evaluator and graph contracts
must arrive before deformers and controls, even though most animators need not see a
node editor. Save/recovery and color/time correctness are prerequisites throughout,
not a polish stage after the character looks convincing.

## Code and evidence audit

| Boundary | Existing evidence | Gap that matters to the character workflow |
|---|---|---|
| Document | `modules/document/include/opentoon/document.h`: stable IDs, drawing references, parented layers, full-pose Transform keys, format 3 | No character/part roles, typed pegs/audio/cameras, saved controls, bind data or compositor graph; no general numeric/discrete property addresses |
| Drawing/import | `adapters/qt/editor_controller.cpp::importImage`, sampled vectors, tiled MyPaint raster and vector selection/clipboard | Qualify transparent part registration and atomic batch/sequence intake; no need to wait for PSD/SVG or full fill topology |
| Hierarchy | `setParent` updates a parent reference; `SceneRenderer::worldTransform` evaluates ancestors | This does not yet prove preserve-world reparenting, permanent versus temporary pivots or independent character identity |
| Animation | Full-pose/channel easing, timeline exposure IDs, visual poses and key blocks | Exposure references are not named, scoped substitution sets; copying keys is not a portable masked character pose |
| Rendering | `adapters/render/scene_renderer.cpp` draws layers in order with QPainter; UI/export reuse it | No texture-warp/deformer backend, image/transform/matte graph, active output camera or explicit managed working-space profile |
| Audio | Rational `sampleAt`; miniaudio is resolved in `conanfile.py` | Not linked/used. UI timer playback is not a device-master audio clock; no waveform, clip mixing or lip sync |
| Storage/recovery | SQLite revisions, immutable media, compression/checksums, migrations and undo | Every new rig/graph/audio payload needs reachability, validation, migration and recovery tests; current snapshots do not prove this |
| Validation | 65 CTest entries plus native authoring/animation and relocated Mac bundle journeys | No deformable speaking-character journey, artist acceptance, production-scale qualification or physical tablet evidence |

See [status.json](../implementation/status.json) for actual measurements and their
limits. Existing subsystems are reused and qualified for their next consumer; the
plan neither restarts them from zero nor assumes a partial catalog item is complete.

## Dependency changes and their reasons

| Previous broad dependency | Replacement | Why this is safe only after the named contract |
|---|---|---|
| Complete P03/P04 before animation, raster and character work | HM-01 properties + HM-02 usable artwork | Annotation, palette textures and analytic fill are independent of rigid PNG cut-out; IDs/space/undo remain mandatory |
| Complete P06 cameras and P07 media before P08 cut-out | HM-03 consumes typed pegs, registration and property persistence | A rigid character needs transform hierarchy and substitutions, not camera multiplane or movie codecs |
| Complete asset libraries/lip detection before P09 | HM-05 consumes stable parts/variants, rest transforms and HM-04 render boundary | Bind IDs and variant compatibility are mandatory; library search, archive transport and automatic recognition are not |
| Complete P09 before any P10 work | Extract HM-04 graph/evaluation kernel first; HM-12 node UI/mattes can follow independently | Deformers and controls need evaluation order; image alpha-over/cutters do not need a deformation solver |
| Complete scripting P12 before controls P13 | Move declarative widgets/sliders/dashboard/portable bindings to P08 | Limited typed drivers can use commands and DAG validation without Python; scripted CTL-006 moves to P12 |
| Audio gated by finished raster P05 | HM-10 needs saved audio assets, rational time, jobs and a measured device clock | WAV playback and waveform generation are independent of paint-engine completeness |
| Lipsync requires an automatic detector | HM-11 joins working audio with named mouth substitutions | Editable timing and mappings provide the workflow; a detector can later propose data through the same validated import boundary |
| Camera bundled with all multiplane/3D views | HM-13 extracts one active orthographic camera | Shared scene-to-output sampling is essential; perspective and parallax are not needed for the first speaking-character shot |
| Professional imports gated by advanced FX/scripting | P16 no longer waits for P15; formats retain their own contract/parser gates | PSD/SVG are valuable follow-ons, but registered PNG parts unblock the first character without a format conversion project |
| Studio/AI depend on unrelated game/FX completion | Remove those coarse phase edges; retain real document/job and format needs | Long-term work remains unimplemented; this is a scheduling correction, not authorization to start it now |

`phase.depends_on` now expresses full-epic completion obligations. HM implementation
entry is controlled by `delivery_slices[].requires`, with a concrete contract on every
edge. A consumer does not start on an unfinished prerequisite merely because a UI
prototype exists. Independent research may proceed without publishing that consumer
as ready. Full P11 still requires the retained P00–P10 obligations.

## Important architecture joins

- **Properties before controls:** stable numeric/discrete addresses, authoring ownership,
  driver conflict rules and deterministic sampling must exist before widgets write to
  rigs. Use a property-address adapter over format-3 full-pose keys; preserve their evaluated behavior without an upfront curve-engine rewrite.
- **Bind before deform:** rest geometry, weights, UVs and per-variant compatibility are
  persistent intent. Rebuilding a derived mesh must not redefine a bind silently.
- **Solver plus renderer:** a bone/curve solver does not render a warped image. HM-05
  must prove both. A regular mesh over transparent artwork avoids requiring arbitrary
  vector-region topology; editable vector sources may use a declared, resolution-limited
  proxy. That is not a claim of analytic vector deformation.
- **Attachments after deformation:** an attached hand consumes the evaluated endpoint
  transform without receiving the arm's deformation twice. Two-bone IK has one declared
  owner/solve order; general pins, feedback and animated constraint switching wait.
- **Substitutions and poses:** held drawing choices differ from numeric interpolation.
  Named roles map poses across compatible instances; incompatible variants/binds reject
  explicitly. Pose sliders have exact endpoints and a declared discrete threshold.
- **Template closure evolves:** first rigid parts/poses, then deformer/controls, then
  nodes/mattes/media. No portable-rig claim until the collector covers every registered
  payload. Folder manifests avoid coupling this to an archive parser.
- **Camera is not viewport navigation:** output framing is persisted and sampled by the
  same evaluator used by export. Editor zoom never becomes a shot camera implicitly.
- **Time and color cannot be postponed:** the audio device clock drives live playback;
  exact rational mapping drives offline output. Restricted linear-sRGB composition and
  legacy appearance preservation precede mattes; full OCIO/HDR follows later.

The proposed contract changes are in [ADR-022](../architecture/adr/022-character-first-delivery.md).
They must be implemented and verified in their slices; this planning change does not
bump the on-disk format or claim that the new APIs already exist.

## Differentiation to test with animators

**Quick Rig** is a new explicit requirement, RIG-015: assign parts, place guides,
preview a suggested FK rig, correct it and commit once. It must produce ordinary
editable hierarchy/controls, support cancel/undo and never depend on image recognition.
Optional deformation/IK recipes appear only after those capabilities pass their gates.

**First-class substitutions and poses** belong to a character, use stable mappings,
have thumbnails/names, include explicit channel masks, and survive reusable templates.
A pose can include mouth/hand choices without overwriting unrelated animation.

**Progressive complexity** provides an Animator workspace with a compact character
picker, published controls, substitutions, poses, timeline and audio. The Rig workspace
exposes binding, hierarchy, drivers and nodes. Both edit the same model; neither is a
separate export or irreversible simplified rig. Properties follows the selected target.

These are intended advantages, not measured superiority. Compare identical registered
character tasks with manual OPEN-TOON rigging and, when available, a licensed Harmony
workflow. Record setup time, corrections, actions, animator errors and successful reuse.
Do not trade away animation range or portability merely to reduce click counts.

## Retained scope and follow-on priorities

All 282 previous requirements, 23 phases and 193 node/family entries remain. RIG-015
adds one requirement: **283 total**, with one primary completion phase each. Early
slices cover declared subsets only. A node/family name still needs an operator spec;
the small HM set does not imply implementing the entire node inventory.

Immediately after the integrated gate, use artist evidence to order review-movie
output, layered PSD/SVG intake, automatic lip proposals, envelopes, 2D pose grids,
foot pins/constraint animation and multiplane. Preserve their actual prerequisites.

Defer exhaustive paperless annotations, specialized drawing/brush features, advanced
palette/textures, docking customization, broad codec/FX catalogs, scripting, morphing,
3D, games, studio, AI and legacy import from the HM schedule. Their primary phase
assignments, acceptance, estimates and explicit scheduling dispositions remain in
`roadmap.json`; none is marked done or deleted. Fix any defect in an existing subsystem
that blocks the shot, regardless of phase, before adding another convenience feature.

## Dependency reuse decisions

Keep C++20, Qt/QML, SQLite, existing MyPaint and locked infrastructure. Use miniaudio
only after a WAV/device-clock spike; its presence in Conan is not adoption. Evaluate
only the required Eigen operations and mesh/render path before deformers. A
triangulator is neither a weight solver nor a texture renderer. Keep libigl advanced
weights, Clipper2 region topology, Skia replacement and docking as evidence-driven
follow-ons. Use the existing Qt image route for the PNG profile. Full OCIO/OIIO/EXR
and broad FFmpeg codec support do not gate it. Rhubarb remains optional after manual
lip sync; a failed detector must never prevent opening or animating a scene.

## Primary references reviewed

These establish reference behavior/library boundaries, not implementation or adoption.
The dependency conclusions and product choices above are OPEN-TOON design judgments.

- [Harmony 25 Drawing Substitutions](https://docs.toonboom.com/help/harmony-25/premium/reference/view/drawing-picker-view.html): selecting thumbnails changes exposed artwork; our character-scoped sets/poses build on a distinct product model.
- [Harmony 25 deformers](https://docs.toonboom.com/help/harmony-25/premium/getting-started/deformation.html): rest setup and animated deformation are distinct; full deformer breadth is beyond the first profile.
- [Harmony 25 kinematic output](https://docs.toonboom.com/help/harmony-25/premium/deformation/rig-kinematic-output.html): attachment semantics motivate explicit post-deformation transforms.
- [miniaudio manual](https://miniaud.io/docs/manual/index.html): device/decoder primitives provide infrastructure; OPEN-TOON owns scene timing and synchronization.
- [Rhubarb](https://github.com/DanielSWolf/rhubarb-lip-sync): English and phonetic recognizers have different intended inputs; measure dialogue results rather than assume language parity.
- [libigl tutorial](https://libigl.github.io/tutorial/): skinning/weight algorithms are candidates for later measured deformation needs, not a complete application engine.
- [FFmpeg formats](https://ffmpeg.org/ffmpeg-formats.html): format/muxer support is distinct from selecting and distributing an actual encoder profile.
