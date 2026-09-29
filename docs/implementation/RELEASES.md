# Experimental releases

## 0.2.0-experimental.56 — working source, operator library subset

- Nodes now opens a compact operator library with descriptions and search by
  operator name or category. Its supported actions add a Drawing, set a
  Normal/Multiply/Screen/Add blend, choose Inside/Outside for an assigned
  cutter, or group a marked Drawing span. Each action uses an existing
  transactional editor command.
- Native Qt Quick checks search Matte and Source categories, apply Multiply
  and Drawing, and undo both changes. The locked macOS suite passes 195/195
  CTest entries. This is a partial NOD-003 library; arbitrary graph-node
  creation and wiring remain open. Source only; no public binary or tag.

## 0.2.0-experimental.55 — working source, group bypass

- Format 32 saves a composite group's bypass state. Nodes offers a compact
  checkbox and Alt-click on its Group output card; restoring the group retains
  every member's settings and order.
- Typed input/output routing preserves an internal cutter used by an external
  target. Pixel, migration/backup, undo/redo, reopen and native Qt Quick checks
  pass; the locked macOS suite passes 195/195 CTest entries. Source only; no
  public binary or tag.

## 0.2.0-experimental.54 — working source, whole-group order

- Drag a Group output card onto a Drawing/Part card to move all group members
  together. Shift-drag places it behind; Back and Front also move a selected
  group as one unit, including across another group.
- The transactional move preserves member order, typed ports, cutter references
  and saved pixels. Controller and native Qt Quick checks cover overlap,
  undo/redo and reopen; the locked macOS suite passes 194/194 CTest entries.
  Source only; no public binary or tag.

## 0.2.0-experimental.53 — working source, composite groups

- Format 31 saves named adjacent Drawing/Part groups. The derived Nodes graph
  exposes image input/output ports; Shift-click groups cards, an inline field
  renames, and Ungroup removes the boundary in one undoable edit.
- Cutter references and output pixels survive grouping and reopening. Graph,
  matte pixel, migration/backup, command and native Qt Quick checks pass; the
  locked macOS suite passes 194/194 CTest entries. Source only; no public
  binary or tag. Nested and reusable general graph groups remain open.

## 0.2.0-experimental.52 — working source, direct behind order

- Shift-drag a Drawing or Part card onto another to place it immediately
  behind the target. Ordinary drag still places it in front; Alt-drag still
  connects a cutter. One command owns each move.
- A two-Part arm/torso fixture and native Qt Quick drag check the visible
  overlap, parent identity, undo/redo and reopened pixels. The macOS suite
  passes 191/191 CTest entries. Source only; no public binary or tag.

## 0.2.0-experimental.51 — working source, composite bypass

- Format 30 saves a Drawing/Part composite bypass. The layer stops painting
  while its image remains usable as a fractional cutter. Nodes exposes a
  compact control and Alt-clickable composite card.
- Graph, pixel, migration and native Qt Quick checks cover Display/Write,
  dependency invalidation, undo/redo, save/reopen and an older-format backup.
  The macOS suite passes 191/191 CTest entries. Source only; no public binary
  or tag.

## 0.2.0-experimental.50 — working source, Milo continuous rig

- Added the owner's Milo SVG/PNG artwork and a saved 17-Part animation study.
  Each arm and leg is one image with a two-segment bone and its middle elbow
  or knee joint; hands and ankles follow the evaluated tips.
- Verified source/rest color agreement, a connected silhouette across all 48
  frames, exact rest return, reopened bent pixels and native Qt Quick open,
  frame change, save and reopen. The macOS suite passes 188/188 CTest entries.
  The older Clockwork examples stay as
  regression fixtures. Source only; no public binary or tag.

## 0.2.0-experimental.49 — working source, matte Display and node cache

- Show source and inverse matte alpha as full-canvas grayscale without
  changing Write or export. The native smoke samples 64/255 and 191/255,
  alongside unchanged fractional final output.
- The bounded revision cache keys intermediate Display images by node ID as
  well as frame, revision and view options. A focused test checks switching
  nodes, reuse and revision invalidation. The locked macOS suite passes
  187/187 CTest entries. Source only; no new public binary or tag.

## 0.2.0-experimental.48 — working source, alternate Display node

- Show a selected image node's output on the main canvas, then return to final
  output. The chosen Display source is view state; Write, saving and export
  keep the final composition. Reordering that changes node identity clears
  the alternate view.
- Native Qt Quick smoke checks red/blue canvas pixels, unchanged Write pixels
  and document revision, reset and stale-node invalidation. The locked macOS
  suite passes 186/186 CTest entries. Source only; no new public binary or tag.

## 0.2.0-experimental.47 — working source, direct node bypass

- Alt-click an Opacity, Apply matte or non-Normal blend card to toggle its
  existing saved bypass through the same document command as the toolbar.
  Alt-clicking the bypassed card restores the retained setting.
- Native Qt Quick pointer smoke verifies cutter bypass, one-step undo/redo
  and re-enabling from the node card. The locked macOS suite remains green at
  186/186 CTest entries. Source only; no new public binary or tag.

## 0.2.0-experimental.46 — working source, node owner selection

- Cutter source, Invert matte, Apply matte and layer composite cards carry
  derived owner identity. Clicking one selects the source or target Drawing
  or Part for the inspector while retaining the node preview.
- Typed graph validation rejects invalid owners. Domain, presentation and
  native Qt Quick click tests pass; the locked macOS build passes 186/186
  CTest entries. Source only; no new public binary or tag. Arbitrary graph
  wiring and group ports remain open.

## 0.2.0-experimental.45 — working source, blend bypass and direct cutter connection

- Bypass a Drawing or Part blend in Nodes while retaining Multiply, Screen or
  Add. Format 29 saves the state; older projects load with bypass off and keep
  a readable source-version backup on first save. The derived graph presents
  a typed bypass node and both output terminals render Normal source-over.
- Alt-drag a Drawing card onto another to bind it as the target's cutter.
  Ordinary dragging continues to reorder. The existing validation and one
  undoable command protect references.
- Fractional pixel, migration, undo/reopen and native Qt Quick pointer tests
  pass on the locked macOS build (186/186 CTest entries). Source only; no new
  public binary or tag. Full editable graph and group ports remain open.

## 0.2.0-experimental.44 — working source, node navigation

- Search the derived Nodes graph by layer name or node kind. Matching cards
  highlight, the view scrolls to the current result, and Next or Enter cycles
  through matches. Search changes only view state.
- Native Qt Quick smoke types a query, finds Write in a narrower window,
  then clicks Next across three Drawing results without altering pixels.
  Source only; no new public binary or tag.

## 0.2.0-experimental.43 — working source, direct node drawing order

- Drag a Drawing card onto another in Nodes to place it immediately above
  the target in composite order. The drop target highlights; one document
  command supports undo/redo and saved order. Locked drawings reject it.
- A three-color pixel fixture checks stacking, invalid targets, undo and
  reopen. Native Qt Quick smoke sends the actual pointer drag across cards
  and verifies the rendered output. Source only; no new public binary or tag.

## 0.2.0-experimental.42 — working source, additive layer blend

- Add saved Add blending for Drawing and Part layers in Nodes. Fractional
  alpha, cutters and painted sources use the same Display/Write graph under
  Legacy and Linear sRGB composition.
- Format 28 preserves older modes with a readable backup. Pixel, migration,
  undo/reopen and native popup-click tests pass; the original 1080p Add study
  measured 50.93 ms/frame in Legacy and 45.22 ms/frame in Linear sRGB on the
  M1 Pro across three frames.
- Source only; no new public binary or tag. General graph editing remains open.

## 0.2.0-experimental.41 — working source, per-clip stereo balance

- Set a clip's left/right balance from the Audio inspector. Center retains
  historical output; the opposite channel is attenuated toward either end.
- Format 27 loads older clips centered and retains a readable backup on
  first save. Exact PCM, invalid-input, copy, undo/reopen, migration and
  native Qt Quick input checks pass on macOS.
- Source only; no new public binary or tag. Hardware presentation and
  broader device quality remain HM-10 gates.

## 0.2.0-experimental.40 — working source, direct frame-aligned audio trim

- Drag the middle handles at either end of a 48 kHz single-pass clip to trim
  at scene frames. The left edge keeps scene and source positions aligned;
  the right edge changes the source out-sample. Escape cancels and release
  commits one undoable edit.
- Rational 24 and 24000/1001 fps tests compare surviving PCM exactly and
  check rollback, undo/redo and reopen. Native Qt Quick smoke drags both
  edges and checks the resulting source samples.
- Source only; no new public binary or tag. Other source rates and repeated
  clips continue to use numeric trim.

## 0.2.0-experimental.39 — working source, per-clip audio solo

- Isolate one or more audio placements from the Audio panel. Mute still wins;
  all other clip waveforms remain visible and dim when a solo is active.
- Format 26 persists solo state and loads earlier clips unsoloed with a
  readable source backup. Domain, migration and native Qt Quick tests check
  exact mix selection, undo and reopen.
- Source only; no new public binary or tag. HM-10 hardware presentation
  qualification remains open.

## 0.2.0-experimental.38 — working source, refined connected toon

- Redraw the original character's front and three-quarter headwear, and use
  2 × 2 coverage sampling for the registered PNG artwork and its independent
  reference composites. The source retains 19 intake roles and 41 drawing
  names; the editable rig retains four continuous single-artwork limbs with
  elbow and knee bones.
- Regenerate the three editable studies and visual evidence. The 480-frame
  connectivity sweep, independent reference comparison with bounded
  compositor-rounding tolerance, 179 CTest entries, deterministic source-art
  checks and native Animator/integrated-shot smokes pass on macOS.
- Source only; no new public binary or tag. Owner artistic approval and
  broader extreme-bend review remain open.

## 0.2.0-experimental.37 — working source, exact 48 kHz clip split

- Split a single-pass 48 kHz clip at an interior scene frame. The two clips
  share the original WAV and preserve the exact canonical 48 kHz PCM mix,
  gain, mute and outer fades with one undo step.
- Domain tests compare complete PCM at 24 and 24000/1001 fps, reject
  unsupported cuts, and reopen the saved project. Native Qt Quick smoke
  clicks the button and verifies its PCM result and undo. The 179-entry
  macOS CTest suite passes.
- Source only; no new public binary or tag. Other source rates and playback
  resampling continuity at the cut remain unqualified.

## 0.2.0-experimental.36 — working source, layer blend modes

- Add saved Normal, Multiply and Screen blending for Drawing and Part layers
  in Nodes. Fractional alpha, cutters and source painting pass through the
  same Display/Write graph under Legacy or Linear sRGB composition.
- Format 25 loads earlier layers as Normal and preserves a readable backup.
  178 CTest entries, render/undo/migration tests and a native popup click
  with save/reopen pass. The original 1080p blend study averaged 49.36 ms
  per Legacy frame and 43.59 ms per Linear sRGB frame on the M1 Pro.
- Source only; no new public binary or tag. Broader node editing remains open.

## 0.2.0-experimental.35 — working source, opacity node bypass

- Bypass selected Drawing or Part opacity from Nodes while retaining its
  setup value and animation keys. The typed graph forwards the unattenuated
  image to cutters, Display and Write; undo and reopen preserve the choice.
- Format 24 defaults older layers to enabled opacity and retains a readable
  backup on first save. Render, migration, a legacy scene without a matte
  and native Qt Quick checks pass.
- Source only; no new public binary or tag. General editable nodes remain open.

## 0.2.0-experimental.34 — working source, saved clip mute

- Mute or unmute individual audio clips without changing their WAV resource,
  placement, trim, gain, repeats or fades. The timeline retains a dim labeled
  waveform. Device preview, scrub and both WAV exports share the saved mix.
- Format 23 defaults older clips to unmuted and keeps a readable backup on
  first save. Exact mix, undo/redo, migration, native click and 173 CTest
  entries pass on macOS.
- Source only; no new public binary or tag. Hardware recovery remains open.

## 0.2.0-experimental.33 — working source, contour-bound continuous toon

- Rebind all four continuous arms/legs and the alternate sleeve in the saved
  rig and 20-second studies to alpha-following meshes with a 40 px joint
  transition. The elbow and knee silhouette stays joined at the bend.
- Regenerate the editable examples and representative stills. A 480-frame
  connectivity sweep, 171 CTest entries, native HM-07 and integrated-shot
  smokes, and current visual-shot input/render measurements pass on macOS.
- Source only; no new public binary or tag. Artistic approval remains open.

## 0.2.0-experimental.32 — working source, reusable audio clip placement

- Duplicate a clip at the playhead without copying its embedded WAV. The new
  placement retains trim, gain, repeats and fades and can be edited separately.
- Exact PCM cues, one-step undo/redo, invalid-input rollback, save/reopen and
  a native Qt Quick button click pass. The macOS suite passes 171 CTest entries.
- Source only; no new public binary or tag. HM-10 hardware qualification remains open.

## 0.2.0-experimental.31 — working source, integrated shot study

- Add an original 20-second editable project joining the continuous four-limb
  toon, nine timed mouth changes, a visible head source cutter for the eyes,
  output camera and a synthetic PCM cue track with sample-based fades.
- An integration test checks exact audio/mouth cue positions, matte graph,
  unchanged WAV bytes, 480-frame length, rendered frames and save/reopen. The
  170-entry macOS suite and native Qt Quick open/screenshot smoke pass.
- Source only; no new public binary or tag. Synthetic cues are timing signals,
  and full audiovisual/artistic qualification remains open.

## 0.2.0-experimental.30 — working source, direct audio fade handles

- Drag the upper fade guide handles on a waveform row to preview and commit
  source-sample fade lengths. Escape cancels; release creates one undo step.
- Native Qt Quick mouse gestures check both handles, exact sample changes,
  undo/redo and cancellation. The 169-entry macOS suite and audio smoke pass.
- Source only; no new public binary or tag. Hardware audible quality remains
  unqualified.

## 0.2.0-experimental.29 — working source, audio fade guides

- Draw the saved fade-in and fade-out endpoints over a clip's timeline row,
  including the full repeated length. The source-peak waveform remains visible.
- Native Qt Quick smoke compares the timeline row before and after setting
  fades; the screenshot was inspected alongside the 169-entry macOS suite.
- Source only; no new public binary or tag. The guides are a visual envelope;
  source peaks do not yet include sample-weighted fade amplitude.

## 0.2.0-experimental.28 — working source, source-sample audio fades

- Set linear fade-in and fade-out durations on a repeated clip in source
  samples. The immutable mixer applies them to device preview and full or
  selected-range PCM WAV output without changing the original WAV.
- Format 22 migrates older clips to zero fades and keeps a readable backup.
  Exact PCM, undo/redo, trim clamping, 169 CTest entries and native HM-10
  smoke pass on macOS.
- Source only; no new public binary or tag. Audible hardware quality remains
  unqualified; the next source build adds the visible fade guides.

## 0.2.0-experimental.27 — working source, node output preview

- Click a derived image or matte node to inspect its evaluated output at the
  current frame. Matte alpha appears as grayscale; the selected preview
  follows edits and frame changes without changing the document.
- Renderer pixel checks and native Qt Quick card clicks verify Drawing,
  Cutter and Apply matte previews. The local macOS suite passes 167 CTest
  entries and HM-12 smoke.
- Source only; no new public binary or tag. General editable nodes remain open.

## 0.2.0-experimental.26 — working source, visible cutter source

- Keep a Drawing or Part cutter visible at its ordered paint position while
  its fractional alpha masks a target. The choice is saved in format 21,
  undoable and available in Properties and Nodes; older projects default off.
- Render/graph, format-20 migration with readable backup and native Qt Quick
  smoke cover pixels, bypass, undo/redo and reopen. The local macOS build
  passes 167 CTest entries.
- Source only; no new public binary or tag. General graph editing and joint
  recipes remain open.

## 0.2.0-experimental.25 — working source, continuous trouser contour

- Redraw each leg as one cubic trouser silhouette with a smoother knee and
  tapered ankle. Hip, knee and ankle registration and the central bone remain
  unchanged; regenerated 48- and 480-frame examples stay editable.
- The 90° regular and contour stress poses, 480-frame connectivity and
  substitution/reopen checks pass. An overly wide knee candidate folded and
  was rejected before this bounded redraw. The local macOS build passes 166
  CTest entries, the deterministic asset test and native HM-07 smoke.
- Source only; no new public binary or tag. Artistic acceptance remains open.

## 0.2.0-experimental.24 — working source, persistent cutter bypass

- Save a reversible cutter bypass in format 20. It keeps the source and
  Inside/Outside setting, shows the uncut target and excludes inactive cutter
  work from the output graph. Re-enabling restores the same fractional result.
- Formats 1–19 default to enabled cutters; a first format-20 save preserves a
  readable source-version backup. Native smoke verifies bypass, undo/redo,
  save/reopen and re-enable. The local macOS build passes 166 CTest entries.
- Source only; no new public binary or tag. General graph editing and joint
  recipes remain open.

## 0.2.0-experimental.23 — working source, typed opacity composition

- Show animated Drawing/Part opacity as a typed node in the derived composition
  graph and edit the existing property from the Nodes panel. Fractional cutter
  alpha follows source opacity before Inside or Outside coverage.
- A one-pixel keyed fixture checks Display/Write, undo and save/reopen; native
  HM-12 smoke checks opacity editing and a real project startup with `--open`.
  Missing startup projects fail with a clear error.
- The local macOS build passes 165 CTest entries. Source only; no new public
  binary or tag. General editable nodes and part-overlap recipes remain open.

## 0.2.0-experimental.22 — working source, band-limited upsampling

- Use the same bounded 32-tap, 1,024-phase rate-conversion kernel for
  upsampling as for downsampling. Equal-rate PCM keeps its direct sample path.
- An 8-to-48 kHz 3 kHz tone retains 0.560 RMS; split and whole output blocks
  match. The existing 96-to-48 kHz anti-alias and callback-period checks pass.
- The local macOS build passes 164 CTest entries and native audio smoke.
  Source only; no new public binary or tag. Broader rate-ratio and hardware
  presentation qualification remain open.

## 0.2.0-experimental.21 — working source, selected audio range

- Export a selected half-open frame interval to PCM WAV. Its sample count
  follows the exact scene rate, and its PCM payload matches the same slice
  of a full-scene mix, including at 24000/1001 fps.
- The native audio smoke checks a cue in a two-frame range. Invalid ranges
  fail before writing; cancellation retains the earlier destination.
- The local macOS build passes 163 CTest entries. Source only; no new public
  binary or tag. Full synchronized PNG/WAV delivery remains open.

## 0.2.0-experimental.20 — working source, anti-alias audio downsampling

- Precompute a bounded 32-tap, 1,024-phase low-pass table for each unique
  source rate above the 48 kHz output rate. Preview and WAV export share
  this deterministic path; original WAV assets and saved format 19 stay
  unchanged.
- A 96-to-48 kHz signal test suppresses a 30 kHz source while preserving a
  1 kHz tone. Repeated cues and split output blocks match; two simultaneous
  96 kHz tracks fit the local callback period. HM-10 hardware presentation
  and broader rate-quality gates remain open.
- Source only; no new public binary or tag.

## 0.2.0-experimental.19 — working source, outside cutters

- Save an inverted cutter choice in format 19. Inside and outside coverage
  preserve fractional alpha, share the typed display/write graph and remain
  undoable in the inspector and Nodes workspace.
- Formats 1–18 load with inside coverage; a first current-format save keeps
  a readable source-version backup. Native smoke checks pixel values,
  save/reopen and bypass/undo. HM-12 remains in progress.
- Source only; no new public binary or tag.

## 0.2.0-experimental.18 — working source, cutter mattes

- Save a Drawing or Part cutter matte binding in format 18. The inspector
  assigns or bypasses a visible source with atomic undo; fractional alpha
  clips the target without painting the source into the final image.
- The same typed graph evaluates preview and write output, with validation
  for source references and character-copy remapping. A resizable Nodes tab
  exposes the derived graph and compact order/cutter edits. HM-12 remains in
  progress pending a general editable graph and joint recipes.
- Local macOS tests and native smoke verify fractional pixels, save/reopen
  and bypass/undo. Source only; no new public binary or tag.

## 0.2.0-experimental.17 — working source, audio repeats and device preview

- Save 1–64 sample-contiguous repeats per trimmed clip in format 17, with
  format-16 migration and a readable source-version backup.
- Preview audio through miniaudio, scrub frame fragments, drag clips in the
  native timeline, reuse exact indexed waveform peaks and export the same
  mix to PCM WAV. HM-10 remains in progress pending hardware synchronization
  and audible-quality qualification.
- The local macOS locked source build passes 154 CTest entries and native
  HM-10 input/export smoke. This working source version has no new binary or
  source tag.

## 0.2.0-experimental.16 — source milestone, audio timing subset

- Save original mono/stereo PCM16 WAV assets and undoable scene clips in format 16.
- Draw sample-aligned waveform rows, edit clip frame position, source trim
  and gain, and export a deterministic 48 kHz stereo WAV mix. Device playback,
  real-time mixing and scrub remain open HM-10 work.
- Keep format-15 migration backups and reject invalid WAV input atomically.
  The macOS locked source build passes 148 CTest entries and native audio smoke.
- Source only; no new public binary or cross-platform qualification.

## 0.2.0-experimental.15 — orthographic output camera

- Add 20 bounded camera behaviors: direct pan/rotate/zoom, constrained gestures,
  animated framing, guides, selected-camera Properties, safe undo/deletion and
  identical preview, reopened and PNG output. See [HM-13 acceptance](HM13-ACCEPTANCE.md).
- Save one explicit output camera in format 7. Format-6 projects migrate with a
  source-version backup; invalid and singular camera framing is rejected.
- Close the bounded HM-04 graph contract with native timed playback/scrubbing,
  fuller alpha/color checks and repeated animated original-art cost measurements.
  This does not complete the wider P10 node-compositor phase.
- macOS locked build: 99/99 CTest entries and the native input/composition/camera
  smoke pass. Source only; no new dependency or Windows/Linux build qualification.

P06 and P10 remain in progress. HM-03 artist-led rigid-character acceptance,
deformation, audio and editable node topology remain on the Harmony Moment path.

## 0.2.0-experimental.14 — speculative compositor preview

- The linear-sRGB canvas prepares its next frame on one background worker from an
  immutable scene snapshot. Current-frame cache misses remain accurate during scrubbing.
- The one-slot pending queue coalesces repeated requests, cancels superseded work,
  rejects stale publication and skips frames that exceed the preview memory budget.
- The original 19-part Clockwork Hello artwork now has exact alpha-coverage and
  Display/Write/reopened-pixel checks, plus a 1080p compositor cost baseline.
- macOS: 95 CTest entries and native UI smoke pass. No new dependency or project
  format change; source only. See [HM-04 evidence](HM04-GRAPH-KERNEL.md) and the
  [benchmark](COMPOSITOR-BENCHMARK.md).

HM-04 and P10 remain partial. Timed native playback/scrubbing, denser animated
scenes and fuller color charts are still required before the graph contract is
accepted. Linux/Windows builds and binary publication are deferred.

## 0.2.0-experimental.10 — vector authoring and animation patterns

- Whole-vector lasso with additive/subtractive selection, select all and invert.
- Independent vector cut/copy/paste between drawings/scenes with preserved colors.
- Alignment, center distribution, stable art-layer stacking and keyboard nudges.
- Batch width, palette, art-layer and fill edits in selection-owned Properties.
- Adjustable corner-aware smoothing and pressure-aware sampled-pencil simplification.
- Line tool, constrained primitives and drawing-local grid/snapping.
- Batch full-pose interpolation/easing and collision-safe key-block repetition.
- Professional project README and expanded English workflows/contracts.
- Verified local macOS arm64 preview, macOS 15 minimum, bundled runtime and notices,
  matching dependency sources/recipes, checksums and verified ad-hoc signatures.

Binary publication was cancelled at the owner's request; this tag distributes source only.
The locally prepared package remains available in the development workspace.

65/65 CTest entries and all native authoring/animation smoke journeys passed. The
release ZIP also passes those native journeys after extraction outside the workspace,
with no external Homebrew or developer-home runtime libraries loaded. Format 3 remains
unchanged; no new application library was adopted. P04/P06 remain partial. Physical
tablets, Intel, clean-machine validation and new Windows/Linux binaries remain pending.
The macOS preview is not Developer ID signed or Apple-notarized. See [ADR-021](../architecture/adr/021-vector-authoring-and-key-patterns.md)
and [installation notes](MACOS-PREVIEW.md).

## 0.2.0-experimental.9 — additive vector selection

- Shift adds and Alt subtracts whole vectors by click or rectangular marquee.
- Sparse groups retain their members when switching Select/Marquee; dotted bounds and Properties show the actual selection.
- Direct group move/scale/rotation keeps enclosed bystanders untouched.
- Duplicate selects only new copies, including overlapping copies; previews cancel and edits undo atomically.
- macOS: 56 CTest entries and native selection/animation workflows passed. Source only; format 3 unchanged; no new dependencies.

VEC-009/P04 remains partial: no lasso, partial contour selection or raster masks. See ADR-020.

## 0.2.0-experimental.8 — canvas motion positions

- Animate → Path: click/drag pose markers on the canvas; double-click the trajectory to add a sampled pose.
- Fixed local drawing reference; parent-aware movement through rotated, negative and nonuniform transforms.
- Screen-space picking, frame label, Shift direction constraint and transactional preview/undo.
- Changes only X/Y; retains timing, other transform channels, easing, artwork and parent animation.
- View changes cancel active path drags; singular parents and locked layers reject edits.
- macOS: 55 CTest entries; native trajectory, hierarchy, reference-stability and persistence checks passed. Source only; format 3 unchanged.

This is a partial ANI-008/P06 implementation. Spatial spline tangents and separate
velocity remain pending. Adding a pose can change interpolation between keys. See ADR-019.

## 0.2.0-experimental.7 — direct pose-key blocks

- Shared selection across Curves, Timeline and Xsheet, with span/toggle/box gestures.
- Move selected diamonds together, Alt-drag to duplicate, drag the right edge to stretch timing.
- Collision-safe local-pose copy/paste between layers preserves pivots/easing without drawings.
- Focus-aware copy/paste, select all, key nudges and Delete; Escape or selection changes cancel previews.
- One undo per committed operation, existing format 3 and no new dependencies.
- macOS: 54 CTest entries plus native group editing, shortcuts and persistence checks. Source only.

P06 remains partial: no independent channel times, cross-layer batch selection,
world-space retargeting or cameras. See ADR-018 for the exact supported scope.

## 0.2.0-experimental.6 — editable combined motion

- All motion is the default: highlight a channel and edit square keys or round handles in place.
- Linked easing shapes all channels together; switch it off for individual handle edits.
- Full-pose diamonds drag in All motion, Timeline and Xsheet; Keys mode supports double-click insertion.
- Time zoom/panning, wider timeline cells, larger hit areas and explicit add/remove controls.
- Clear removes selected exposures and animation keys together, with one undo.
- Double-click pencil/polygon segments to insert points; Delete/Backspace removes a selected point.
- macOS: 50 CTest entries; native combined-curve, key drag/add, point and Clear workflows passed. Source only; format remains 3.

P04/P06 and P11 remain partial. Independent channel timing, analytic contour geometry,
spatial velocity, cameras and drawing morphing remain open. See ADR-017.

## 0.2.0-experimental.5 — visual animation and precise picking

- Animate (A): move, scale and rotate layer poses on the canvas, with initial anchors, live previews and one undo per gesture.
- Integrated Bezier handles, channel easing/overshoot presets, double-click graph keys and a visible canvas trajectory.
- Constant eight-screen-pixel vector picking margin; distinct tool cursors and contextual move/resize/rotate/point feedback.
- Live point-edit previews, usable thin-line handles and a scrollable tool strip.
- Compact shared controls, a single curve toolbar, optional Values fields and a working draggable panel divider.
- Format 3 curve persistence, original backups for schema 1/2 upgrades and current-format compaction.
- macOS: 48 CTest entries plus native mouse pose, point and Bezier workflows. Source only.

Drawing morphing, contour Bezier geometry, independent channel key times, separate spatial velocity and cameras remain open. Older editors cannot open format 3; use the migration backup with them. See ADR-016.

## 0.2.0-experimental.4 — direct editing workspace

- Removed separate selection, timing and curve dialogs.
- Canvas box handles move, scale and rotate with live preview; Shift constrains
  proportions/snaps angle, Escape cancels, release creates one undo command.
- Properties follows selected objects/regions or explicitly selected layers.
  Numeric fields rebind after edits; single vectors expose width, fill, art and color.
- Curves and timing controls remain in the resizable bottom workspace. Timeline and
  Xsheet range-end handles stretch timing directly.
- Retina transform overlay alignment corrected during visual verification.
- macOS: 43 CTest entries plus native handle/Properties/timing journeys. Source only.

Raster free transforms currently use nearest-neighbor sampling. Rotated ellipses
become 128-point editable polygons. Lasso, soft masks and production-scale transform
performance remain pending; see ADR-015. No phase completion or binary installer claim.

## 0.2.0-experimental.3 — P04/P05 source release

- Rectangular selection of fully enclosed vector strokes and/or raster pixels.
- Integer move/duplicate/delete, horizontal/vertical flips and lossless 90° rotation.
- Shared immutable tiles, alpha-safe overlapping moves, guarded canvas bounds and undo.
- Raster brush opacity; eraser strength independent of palette alpha.
- English selection controls, shortcut M and explicit mixed-media behavior documentation.
- macOS end-of-block validation: 41 CTest entries and native selection move/cancel,
  undo/redo and pixel-identical save/reopen. No Windows/Linux builds or installers.

P04/P05 remain partial: no topology fill, lasso, soft masks, arbitrary selection
rotation/scaling, brush texture management or ABR importer. Imported image assets are
excluded from this selection tool; vector geometry remains editable.

## 0.2.0-experimental.2 — source release

- Explicit Setup/Animate modes and guarded Auto key.
- Keyed/interpolated/held inspector state and previous/next key navigation.
- Pose-channel curve graph with numeric edits, mouse dragging and single-command undo.
- Key-only multi-layer retiming with collision rejection and unchanged exposures.
- English usage and animation contracts; no format or dependency changes.
- Verification at block end on macOS: 36 CTest entries, native drawing/range checks
  and curve drag/undo. Fixed a QML final-property naming conflict during validation.
- Native multi-OS CI is manual; no Windows/Linux build or installer for this source release.

P11 remains incomplete. Keys still store whole poses; independent channel timing,
editable tangents, cameras, audio, advanced rigs, deformation and nodes remain open.

## 0.2.0-experimental.1

An incremental development release on the path to P11. It does not complete P11 or
establish production equivalence with Harmony Premium. The full phase obligations
remain in [STATUS.md](STATUS.md) and the canonical roadmap.

### Available changes

- Multi-layer timeline/Xsheet ranges; linked or independent drawing paste, keys-only
  paste, cycles, timing on ones/twos/threes, guarded stretch and Alt-drag overwrite.
- Scene markers and paginated Xsheet PDF export (200-page cap).
- MyPaint raster ink, soft, dry, smudge and eraser presets, with mouse or pressure input,
  per-gesture undo, cancellation and the same pixels in preview, save/reopen and PNG export.
- Sparse immutable raster tiles; unchanged media shared by undo and saved revisions.
- Compressed SHA-256-checked resources in project format 2, version 1 migration backups,
  explicit project compaction with full backup and background recovery snapshots.
- Pinned dependency additions and source/license records, native range/brush workflow
  checks, migration/corruption/compaction tests and a reproducible synthetic 4K benchmark.

### Verification

[Source commit c07414f passed CI](https://github.com/mijim/OPEN-TOON/actions/runs/35531337766)
on Windows Server 2022, macOS 14 and Ubuntu 24.04 with Qt 6.8.3. The suite has
33 CTest entries on macOS/Linux and 32 on Windows (the POSIX process-termination
case is excluded). Linux additionally passes 27 sanitizer cases. macOS runs native
mouse/pressure, raster pixel round-trip, timeline selection/Alt-drag and undo checks.
The subsequent release commit updates documentation and evidence only.

### Compatibility and limits

The previous editor cannot open format 2. A version 1 project's first new save leaves
an adjacent `.pre-v2.bak` that remains readable by the previous version. Keep that
backup until the upgraded project is accepted. Compaction also preserves the original
history in an adjacent backup.

This release remains a source/build-tree application. Public standalone installers,
signing/notarization, physical tablets and clean-machine deployment are unqualified.
The advanced vector, camera, audio/video, rig library, deformation and node/color
pipeline requirements are still open. Raster import transforms, ABR/texture support
and configurable brush dynamics are not yet available. Consult the guide for exact
available controls.

The screenshot `raster-editor.png` is generated by the native input workflow using procedural test strokes. It contains no imported artwork or private project content.
