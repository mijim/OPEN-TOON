# Document model and open format

This is a proposal, not an implemented format. Designing the model before drawing screens prevents the timeline, Xsheet and node editor from storing three different scenes.

## Entities and invariants

| Entity | Minimum data | Invariant |
|---|---|---|
| Project | ID, name, scenes, libraries, configuration | Paths are not identities |
| Scene | ID, settings, layers, graph, tracks, references | A revision represents a coherent state |
| SceneSettings | Resolution, pixel aspect, rational FPS, range, color | Positive values and known bounds |
| DrawingAsset | ID, version, vector/raster content, sublayers | Can exist without being exposed |
| ArtLayer | Semantic type and content | Underlay → Color → Line → Overlay order |
| ExposureSpan | Start, exclusive end, DrawingId or empty | No ambiguous track overlaps |
| AnimatableProperty | Base value, type, optional curve | Distinguishes rest, local and evaluated values |
| Keyframe | Time, value, interpolation, tangents | One value per time/property after collision resolution |
| Palette/Swatch | IDs, values or resources, name | Equal RGBA does not mean equal identity |
| Rig | Nodes, hierarchy, rest state, controls | Acyclic hierarchy and consistent binding |
| CompositorGraph | Nodes, ports, connections | Valid types and no illegal evaluation cycles |
| AudioClip | Asset, scene start, in/out, gain | Trimming references audio without destroying it |
| AssetRef | Hash, type, metadata, provenance | Hash identifies immutable content |
| Revision | ID, parent, manifest, timestamp | Never publishes references to missing blobs |

A drawing shared by several exposures is one resource. Removing a cell does not delete it. Duplication creates another ID; cloning retains an explicit relationship. Color identity remains stable when its value changes, allowing production-wide recoloring without RGB searches.

## Time

Use zero-based frames internally and half-open intervals `[start, end)`. The UI may display frame one by default, with conversion in one place. Store FPS as a reduced rational such as `24000/1001`, not the approximation `23.976`. Audio uses sample indices and explicit rational conversion; do not round subframe interpolation prematurely.

Four drawings on twos occupy `[0,2)`, `[2,4)`, `[4,6)`, `[6,8)`. They display as frames 1–8 and export to eight images. A frame-rate change offers preservation of frame count or duration with specified resampling rules. One transaction updates all affected tracks and markers according to the operation.

Playback uses a monotonic clock and synchronizes to the audio clock when sound is active. Dropping frames for responsiveness is a visible preview policy, never a reason to omit final-render frames.

## Geometry, raster and color

**Vector:** curves and contours retain editable geometry, width profiles, styles and region identities. GPU tessellation is a derived cache. Specify winding/fill rules, document-unit tolerances and self-intersection behavior. Skia or another library must not become the sole serialized drawing schema.

**Raster:** sparse tiles per layer/drawing with explicit alpha and recorded precision. Changing a tile creates a version; empty areas need no huge maps. Brush textures have provenance and licenses independent of their presets.

**Compositing:** scene profile with a linear working space and floating-point format where appropriate; display transformation is separate. Declare premultiplied or straight alpha at each boundary and convert deliberately. Do not apply gamma twice or treat hidden RGB beneath zero alpha as opaque color. An artistic mode operating in nonlinear space must declare that behavior.

## Proposed container

```text
shot.otoon/
  manifest.json       # formatVersion, projectId, sceneId, headRevision
  document.sqlite     # indices, revisions, scenes and transactional metadata
  blobs/sha256/...    # immutable drawings, tiles, audio and source resources
  previews/...        # optional and regenerable
```

Readable interchange export produces versioned JSON and resources for diagnostics and external tools. Do not maintain JSON and SQLite as two simultaneously editable canonical sources. The manifest provides version and location; canonical state is published as a coherent database/blob revision.

Transport packages are created from closed snapshots with inventories and hashes. Do not edit an open ZIP directly or package SQLite with an active WAL. Use backup/checkpoint and controlled closure before packaging.

## Save protocol

1. Capture the revision to save without blocking throughout disk IO.
2. Write new blobs to temporary files, verify hashes, perform required flushes and rename within the same volume.
3. Commit metadata/revision transactions referencing those already durable blobs.
4. Publish the manifest through a platform-compatible atomic replacement; synchronize the directory where required.
5. Keep the previous revision recoverable and remove orphaned temporary files in a later safe pass.

[SQLite atomicity](https://sqlite.org/atomiccommit.html) does not itself make external files transactional. The format must detect a manifest lagging behind the database and resolve it to an intact revision. Never delete old blobs in the same step that publishes a new revision. Garbage collection uses all retained revisions, pinned exports and recovery journals as roots.

Required checks: interruption at every boundary, disk full, denied permissions, missing resources, continued editing during save and opening the same scene twice. SQLite synchronization settings that sacrifice durability require an explicit decision and cannot be described as guaranteed recovery.

## Evolution and migration

- `formatVersion` evolves independently of the application version.
- Migrations are deterministic and operate on a copy or new revision, preserving the original.
- Unknown extension blocks are retained and flagged, with type and size limits.
- An older reader lacking essential semantics opens read-only or rejects the file; it does not silently save with data loss.
- Validate dimensions, counts, archive paths, references and cycles before allocating memory.
- Retain fixtures for every published version and test semantic round trips.

## Interchange formats

| Profile | Intended purpose | Losses or limits to declare |
|---|---|---|
| PNG and sequences | Basic image delivery with alpha | No rigs, layers or curves |
| WAV/PCM | Reference audio and mixes | No animation-document representation |
| SVG | Interoperable vector subset | Filters, fonts and complex styles may need conversion |
| PSD | Layout and paint layers | Modes, masks, smart objects and effects may not transfer |
| EXR | Float compositing and passes | Channel, premultiplication and color conventions required |
| Video | Preview and delivery | Codec and alpha availability depend on profile/build |
| OTIO | Cuts, tracks, timing and references | Not a graphics scene or rig container |
| FBX/Alembic/USD | Professional interoperability research | Evaluate reader, license, animation, materials and axes separately |

Each adapter returns a report of preserved, baked, omitted and failed elements. Accepted imports retain the original as an asset or provenance reference according to policy. A `.svg` or `.json` extension is not a promise that all another application's behavior fits in the format.
