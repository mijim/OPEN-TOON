# Experimental editor guide

OPEN-TOON currently supports an offline vector and raster animation workflow. It is an experimental editor, not the P11 production release. All first-party UI is English; scene and layer names can contain other languages.

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
- **Raster ink:** select the filled-circle tool, then choose Ink, Soft, Dry, Smudge or Eraser from the preset menu. Paint with the mouse or pressure input. Each gesture is undoable; Escape cancels the preview. Raster pixels retain the color painted and do not recolor with palette edits. Imported images remain separate; paint appears above the image and below vector art.
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

Every 60 seconds, a modified document is saved to a recovery file. On a later launch, recovery can open that snapshot as an unsaved scene. Save it under a chosen name. Autosave runs from an immutable snapshot in a worker, so newer edits stay unsaved until the next save. Manual saves remain synchronous. Recovery from operating-system power loss and multiple simultaneous application instances has not been qualified.

Format 2 compresses and shares media between revisions. The metadata limit is 64 MiB and total decoded media is limited to 512 MiB. Undo snapshots share unchanged media; vector metadata is copied. Opening a version 1 project remains supported. Its first save creates a `.pre-v2.bak` copy before upgrading. The old editor requires that backup to reopen the original format.

**Scene → Compact project history** retains the chosen number of newest saved revisions and removes unreachable media, after creating a full `.pre-compact.bak` backup. Save pending edits first. This is an explicit operation and is not part of autosave.

## Timeline ranges

Drag over cells to select a rectangular frame/layer range. **Edit → Timeline range** provides copy, paste (linked exposures, independent drawings, keys, or all), insertion, repeats, stretch and timing on ones/twos/threes. Copy/Paste shortcuts work when the timeline has focus. Paste starts at the playhead and can extend the scene. Copy a range, open another scene in the same window and paste to create independent drawings with remapped palette IDs. The clipboard is local to that window.

Alt-drag inside a selection to move it; the shaded destination shows the overwrite range. Escape cancels a pending move. Stretch refuses any compression that would remove a drawing or merge keys. Timing on ones/twos/threes removes gaps within the selection and overwrites the resulting destination span. Locked selected layers reject the whole edit.

Use **Edit → Scene marker** to label the current frame; an empty name removes its marker. **Export Xsheet PDF** writes a paginated sheet, limited to 200 pages to keep the synchronous operation bounded.

## Export and limitations

PNG export evaluates an immutable snapshot, so later edits do not change the running export. Cancel stops between frames. `manifest.json` records the rational frame rate, frame count and completion/cancellation/failure state. A cancelled or failed directory contains partial output and must not be treated as a complete sequence.

Image import loads a single image up to 4096 × 4096, subject to scene memory/save limits. Image sequences, layered PSD, audio, video output, lip sync, deformation, node effects, OCIO, reusable rig libraries and production installers are still pending. Consult the [phase status](STATUS.md) for the complete boundary.

## Animation edits and curves

Choose **Setup** in Properties to change rest values without creating keys. Existing
keys retain their poses; the canvas continues to display the evaluated animation.
Choose **Animate** to edit the current key. Enable **Auto key** to create a full-pose
key when editing an unkeyed frame, or press **Add key** explicitly. With Auto key off,
unkeyed edits are rejected. The inspector labels keyed, interpolated and held poses.

**‹ Key / Key ›** navigate the selected layer. **Curves** selects the integrated function editor in the bottom workspace.
Choose a channel, click the graph to scrub, or drag a key to adjust its frame and
value. Escape cancels a drag. The frame/value/interpolation fields and **Apply key**
offer precise editing. **Delete** removes the selected pose key. Colliding keys and
invalid values are rejected; the message appears in the status bar. Undo restores
an entire drag or numeric edit.

Keys currently contain complete poses: a value edit affects only that field, while
moving a key or changing Linear/Hold/Smooth interpolation affects all eight channels.
Smooth has fixed easing slopes; editable tangents and independent channel keys remain
planned. Frames shown in the interface start at 1.

Select a range and layers in the timeline. Drag its right edge (bottom edge in the
Xsheet) to stretch timing; Alt-drag inside to move. **Timing tools** exposes clipboard,
repeat and drawing-step controls within the workspace. Drag the separator above the
timing tabs to resize the lower panel. Timing edits are undoable.

## Rectangular vector and raster selection

Press **M** or choose **Marquee**. Select **Vectors**, **Raster pixels** or
**Vectors + raster** in the top toolbar. Drag a rectangle around your artwork.
Vectors must fit completely, including stroke width; crossing strokes stay untouched.
The rectangle selects raster pixels exactly. Imported image assets are excluded.

Drag inside the rectangle to move it. Drag an edge/corner handle to scale, or the
circle above the box to rotate. Artwork previews while dragging and commits on
release. Shift preserves corner proportions or snaps rotation to 15°; Escape cancels.
The top toolbar provides Duplicate, Flip H, Flip V and Deselect without opening a
dialog. Delete/Backspace removes a selected region while the canvas has focus.

**Properties** follows selection. Click a vector with **V** to see and edit its bounds,
stroke width, fill, art layer and palette swatch. A marquee shows selection bounds.
Rotate-by applies an additional rotation. Select a layer row/name to inspect its layer
transforms and animation; with no target selected, Properties shows selection guidance.
Layer animation and object geometry are distinct targets. Each edit is one undo step.

Raster selections retain transparency. Overlapping destination artwork is composited
source-over; transparent source pixels do not clear destination pixels. Moving
nontransparent pixels outside the raster canvas rejects the complete edit. Large
edits exceeding the 128 MiB mutable tile budget also reject safely. Free raster rotation/scaling currently use nearest-neighbor resampling and a
16-million-output-pixel limit; lasso and feathering remain pending. Selection operations edit the shared
drawing: all its exposures change. Duplicate the drawing first to make it independent.

With a raster brush selected, use **Opacity** in the toolbar to control paint or
eraser strength. The eraser does not depend on the selected palette color's alpha.

Rotated rectangles become editable polygons; rotated ellipses use a 128-point polygon
approximation. Nonuniform scaling applies an average stroke-width scale. These tools
are experimental and do not yet provide analytic curve-preserving transforms.
