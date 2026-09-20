# Experimental editor guide

OPEN-TOON currently supports a small offline vector animation workflow. It is an experimental editor, not the P11 production release. All first-party UI is English; scene and layer names can contain other languages.

## Make a short animation

1. Open the application. The new scene is 1920 × 1080, 24 fps and 48 frames.
2. Select **Pencil** (`B`) and draw with the left mouse button. Use **Size** to adjust width. A mouse uses constant pressure; a supported pen event supplies pressure.
3. Click another timeline frame and choose **New drawing**, or double-click a cell. New drawings hold for two frames. A held exposure refers to the same drawing: editing one held frame updates every exposure of that drawing.
4. Use **Duplicate drawing** to make an independent copy, or **Hold for 4 frames** to extend the current drawing. Enable onion skin (`O`) to see neighboring distinct drawings.
5. Press **Space** to play or pause. Left/Right step through frames. Timeline and Xsheet display the same exposure model.
6. Choose **Save** and a `.otoon` filename. Reopen from the Scene menu. **Export** writes a PNG sequence to a new timestamped subdirectory of the selected folder.

The built-in bouncing-ball example provides 24 distinct drawings exposed on twos. It contains only project-generated geometry and may be reused under the repository license.

## Drawing and view controls

- **Pencil:** sampled vector centerline with round segments and pressure-weighted width.
- **Eraser:** cuts sampled strokes approximately. Analytic shape erasing and region topology are not implemented.
- **Rectangle / Ellipse:** drag to create a primitive; enable the filled option for a solid shape.
- **Select:** select and drag one stroke. Delete removes it. There is no lasso or multi-selection yet.
- **Edit points:** drag individual sampled points. **Smooth selected stroke** averages interior points while preserving endpoints.
- **Recolor:** assign the selected palette swatch to a stroke or shape. It does not flood-fill arbitrary enclosed regions.
- Pan with the middle mouse button or trackpad scroll. Ctrl+scroll zooms. `F` fits the canvas. View rotation and mirror do not change the document or exported artwork.
- Escape or loss of window focus cancels an unfinished drawing gesture.

Physical tablet validation remains pending. Pressure routing has a synthetic event test. Tilt, barrel-button bindings, eraser-end behavior, touch gestures and device-specific profiles remain unimplemented/unverified. Mouse drawing requires no tablet.

## Layers, colors and animation

Layers can be renamed, reordered, hidden, locked, soloed, duplicated or cloned. Duplicating makes drawing copies; cloning shares drawings. A solo parent includes visible descendants. Locks protect drawing and transform edits; structural scene edits can still retime layers.

The four art categories are Underlay, Color, Line and Overlay. Their drawing order is fixed. This is not yet a full art-layer manager.

Click a swatch to select it. Double-click to edit its color; all strokes referencing that ID update. The document retains color IDs across save/reopen. Palette import, variants, gradients and managed color are pending.

The inspector edits position, rotation, scale, opacity and pivot. Add transform keys with Linear, Hold or Smooth interpolation. If a layer already has keys, editing a transform inserts or updates a key at the current frame. Parent layers apply inherited transforms and opacity. Reparenting does **not** preserve the world-space pose automatically. There is no camera, curve editor, IK or deformer yet.

## Saving and recovery

Each manual save appends a complete revision. Scene history can restore a saved revision as an undoable edit. A stale writer is rejected if another writer changed the revision on disk; save a separate copy or reopen to resolve it.

Every 60 seconds, a modified document is saved to a recovery file. On a later launch, recovery can open that snapshot as an unsaved scene. Save it under a chosen name. Autosave is synchronous and can pause large scenes. Recovery from operating-system power loss and multiple simultaneous application instances has not been qualified.

The current format limits each serialized revision to 64 MiB. Inline image bytes expand when represented as JSON. History is not compacted, and undo keeps full snapshots under an approximate retention budget. These are prototype limits, not production storage targets.

## Export and limitations

PNG export evaluates an immutable snapshot, so later edits do not change the running export. Cancel stops between frames. `manifest.json` records the rational frame rate, frame count and completion/cancellation/failure state. A cancelled or failed directory contains partial output and must not be treated as a complete sequence.

Image import loads a single image up to 4096 × 4096, subject to scene memory/save limits. Raster painting, image sequences, layered PSD, audio, video output, lip sync, deformation, node effects, OCIO, reusable rig libraries and production installers are still pending. Consult the [phase status](STATUS.md) for the complete boundary.
