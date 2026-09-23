# ADR-023 — Harmony Moment foundation contracts

Status: accepted engineering contract for HM-00, 2026-09-22. The project owner
accepted the reference material and rubric on the same date; see the
[HM-00 review record](../../implementation/HM00-REVIEW.md). This ADR alone changes
no application code, schema version or library lock.

## Existing boundary and reason for a contract

Format 3 has document-global 64-bit IDs for layers, drawings, strokes and swatches;
`Layer.parent` forms a checked hierarchy and `Layer.keys` stores full transform
poses. `FrameRate::sampleAt` maps frames to sample indices by integer division.
`Session::apply` validates a replacement document before publishing an undoable
snapshot. SQLite stores complete revisions and media hashes, and creates backups
on old-format migration. QPainter renders ordered layers, with no character roles,
graph or explicit linear-light compositing profile. These useful baselines do not
prove a saved rig property, stable substitution or deformation result.

## Identity and property ownership

- Every persisted entity uses a nonzero, document-global `Id`; allocate monotonically
  and never reuse an ID within a document. Renaming, reorder and layer parent edits
  retain identity. Typed entity kinds distinguish character, part, peg, drawing,
  audio clip, graph node and camera at command boundaries, even though the current
  scalar ID allocation can remain the storage primitive.
- A property address is a typed `(entity kind, entity ID, property kind)` value, not
  a UI label or JSON path. The initial property kinds are the eight existing
  transform fields plus a held drawing-choice kind when HM-03 consumes it. Numeric
  and discrete kinds reject incompatible values; a dangling ID is an error. Do not
  serialize empty future payloads merely to reserve names.
- Rest/setup values belong to the saved rig. Authored keys belong to animation.
  Evaluation produces a read-only derived value. Selection, hover, workspace
  layout and temporary command preview belong to view state. A control driver
  owns one target property at a time; direct editing of a driven target must either
  edit its source or explicitly unbind the driver through one undoable command.
- Duplicating or templating a character remaps the dependency closure of IDs and
  rejects unresolved external references. Shared drawings remain explicit shared
  resources; separate character instances cannot accidentally share control or
  substitution state.

## Spaces, time and color

- Document pixels use x right, y down, origin at the upper left; integer pixel
  coordinates address pixel corners and sample centers are `n + 0.5`. Geometry
  passes drawing-local → part-local → peg-parent → scene → output-camera → device.
  Display pan/zoom never changes the saved camera. Current transform composition is
  translate(position + pivot), rotate clockwise in y-down coordinates, scale,
  translate(-pivot). Negative scale is valid. Preserve-world reparenting must either
  retain the complete affine result or reject shear/singular cases without editing.
- Time is zero-based with half-open `[start,end)` exposures. Frame rate is a positive
  rational; reduce new values for identity, use checked 64-bit intermediates, and
  define sample boundary as `floor(frame * denominator * sampleRate / numerator)`.
  A 480-frame 24 fps shot ends at sample 960,000 at 48 kHz; the same 480 frames at
  24000/1001 end at 960,960. It therefore runs 20.020 s, not exactly 20 s. The
  reference music is long enough for both; dialogue timing has separate cue assets
  at 2,000 and 2,002 samples per frame. The 24 fps export trims at 960,000. At ten
  minutes the nearest fractional-rate frame differs by less than one frame; no
  repeated floating-point accumulation is allowed in the media adapter.
- Imported PNG input is straight-alpha RGBA8 with sRGB interpretation and explicit
  registration. Existing format-3 QPainter output remains a named legacy visual
  profile. New node composition will use linear-sRGB premultiplied working values;
  a conversion from legacy appearance must be explicit, versioned and compared to
  goldens. Zero-alpha RGB cannot leak into compositing, and display transforms do
  not change saved colors or swatch IDs.

## Persistence, commands and evaluation

- The first new persisted semantic field requires format 4, a format-3 migration
  fixture and a backup whose suffix reflects the **source** version. Older readers
  must reject the newer format rather than discard unknown rig data. A failed save,
  migration or missing referenced blob leaves the prior revision recoverable. Test
  duplicate IDs, cycles, invalid property kinds, corrupt assets and interrupted
  transactions before accepting HM-01.
- UI gestures call application commands. A command owns all document mutations in
  one transaction, validates its candidate, then publishes one immutable revision
  and one undo entry. Preview/view state cannot mutate saved rest geometry. Background
  jobs consume a revision snapshot and publish only when the request still matches
  that revision. This applies to drawings, variants, bind weights and output media.
- The eventual solve order is authored properties → declarative drivers → peg
  hierarchy → bind/deformation → attachment endpoints → output camera → image
  composition. The transform hierarchy and compositor DAG have separate validation;
  an image node does not become a parent peg. No solver, graph payload or binding
  schema is added until its named consumer slice.

## Measurable reference profile

Fixture: [Clockwork Hello](../../../tests/fixtures/harmony-moment/README.md),
1920 × 1080, 480 frames, 19 parts, original registered PNGs and two timing clips.
Development host: macOS 15.5, arm64 Apple M1 Pro, 16 GiB RAM. Hardware is recorded
to make later measurements interpretable; no performance measurement is inferred.

Proposed HM acceptance budgets on this host: cached 1080p/24 fps preview at its
declared preview quality; p95 control-to-preview latency at most 50 ms; resident
memory at most 2 GiB for this fixture; waveform/scrub error at most one displayed
frame; long-audio clock drift at most one frame over ten minutes; exact PNG frame
count and PCM sample count for each time variant. Offline output must be
deterministic after reopen. If the mesh/render spike cannot meet the budget, record
quality mode, bottleneck and an explicit revised target before broadening scope.

## Artist acceptance and later workflow evidence

The owner approved the six-beat reference script and stills for HM-00. A second
animator must evaluate the eventual workflow on an actual saved scene in HM-15.
The rubric asks for original-art and PNG-part intake, a
rigid pose and view switch, two deformable arm chains, an attached hand, twelve
published controls, corrected mouth substitutions against both timing clips, three
overlap/matte cases, camera framing, 480-frame preview/export, save/reopen and
second-scene rig reuse. Record setup time, corrections, missed hits, hidden state,
unexpected pose changes, recovery from missing assets, and output mismatches.
Passing that later workflow review cannot be replaced with the synthetic test or
the HM-00 owner sign-off.

## Alternatives deferred

No giant all-domain schema, independent channel-key rewrite, Skia/Qt RHI migration,
full OCIO manager, native movie encoder, auto lip detection or tablet qualification
is justified by this fixture. These remain explicitly owned by later slices/phases.
