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
- **Select:** select and drag vectors. Shift-click adds strokes; Alt-click subtracts. Delete removes the selected group. Marquee selects enclosed whole strokes; lasso remains pending.
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

Format 2 compresses and shares media between revisions. The metadata limit is 64 MiB and total decoded media is limited to 512 MiB. Undo snapshots share unchanged media; vector metadata is copied. Opening version 1/2 projects remains supported. Format 3 adds channel Bézier easing; the first save of an older schema creates a `.pre-v3.bak` copy before upgrading. The old editor requires that backup to reopen the original format.

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
Smooth has fixed default easing slopes; custom channel Bézier handles are available. Independent channel keys remain
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

With **Vectors**, Shift-drag a marquee to add strokes or Alt-drag to subtract them.
With **Select (V)**, use Shift-click / Alt-click instead. Alt takes priority if both
modifiers are held. Repeated additions do not duplicate membership. Switching between
V and M keeps a vector group selected; changing the media filter clears the selection.
Dotted object bounds show group members, and Properties reports their actual count.
A normal click on an unselected stroke inside the group box selects that stroke.
Raster/mixed additive masks and lasso are not available yet.

Duplicate selects only the new copies, including when they overlap the originals.
Moving or deleting that group leaves unselected strokes inside its box unchanged.


Drag inside the rectangle to move it. Drag an edge/corner handle to scale, or the
circle above the box to rotate. Artwork previews while dragging and commits on
release. Start dragging a handle, then hold Shift to preserve corner proportions or
snap rotation to 15°; Escape cancels. Shift/Alt held before pressing start selection
addition/subtraction instead.
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


## Animate visually (A)

1. Draw your artwork. Expose it across the desired frames using the timeline's hold
   controls; pose keys do not extend drawing exposures automatically.
2. Select the layer and choose **Animate** (A), or **Pose on canvas** in Curves.
3. Move to the desired frame and drag the box to move, its handles to scale, or its
   top circle to rotate. Shift constrains proportions or snaps angles; Escape cancels.
4. Release to record one pose key. A first later-frame edit adds an unchanged frame-1
   anchor. The Animate tool always records keys; the inspector's Auto key setting
   continues to govern numeric edits. Select (V) edits drawing geometry instead.
5. Open **Curves**, select a channel and its starting key, and drag the round handles
   to shape the outgoing transition. **Overshoot** gives an editable starting point.
   Double-click the graph to add a key; use a middle key to make a bounce when the
   start and end have the same value. Numeric controls remain available for precision.

The dashed canvas trajectory follows a drawing reference point. **Path** enables
direct key-position editing; independent spatial Bezier handles and a separate
velocity curve remain pending. Transform interpolation
moves the layer; it does not morph one vector drawing into another. Morphing remains P14.

## Picking and cursor feedback

Thin vector strokes have an eight-screen-pixel selection margin at any zoom. Hollow
shapes select by their outlines; filled shapes also select through their interiors.
Picking targets the active layer and respects art-layer paint order. The cursor shows
the active tool, becomes a hand over movable artwork, changes direction on resize
handles and shows rotation or point-edit feedback where applicable. Locked layers
and playback show unavailable feedback. Edit points previews the changed stroke
while dragging. Opposing middle handles hide on very thin boxes to keep moving usable.


## Compact workspace and panel sizing

Drag the thin horizontal separator with the central grip above the Timeline/Xsheet/Curves
tabs upward to enlarge the lower panel, or downward to return space to the canvas.
Double-click the grip to restore the default height. The panel follows the window size
and retains a usable canvas minimum.

Curves keeps its actions in a single compact toolbar. **Values** reveals the optional
frame/value/base-interpolation fields; hiding them gives that space back to the graph.
Hover the question mark for editing gestures. Buttons, dropdowns, number fields and
checkboxes use the same compact styling throughout the workspace.


## All motion, point editing and Clear

Curves starts on **All motion**. It shows every changing local transform channel of
the selected layer, normalized to its own fitted display range. Click a name in the
legend to highlight and edit that curve while keeping the others visible.

- Drag a **square** to change its channel value and key time. Keys still store complete
  poses: moving a key in time moves its other channels too; their values stay unchanged.
- Select a starting key, then drag the **round handles** to shape its outgoing easing
  or overshoot. **Link easing** starts enabled and applies the handle's transition to
  all pose channels. Turn it off to adjust only the highlighted channel.
- Drag a **diamond** in the bottom Keys lane to change pose timing without changing values.
- Double-click in the plot to add a key at that time/value. Double-click in the Keys
  lane to record the evaluated pose instead. **+ Key** records at the current frame.
- Use **+ / −** to zoom time around the current frame, the lower scrollbar to pan,
  and **Fit** to restore the entire timeline and fit values. Drag the workspace divider
  upward for a taller graph. Escape cancels a drag; Undo reverses one committed gesture.

Linear, Ease and Overshoot presets affect all channels in All motion. Link easing
controls manual handle drags only. The dropdown's individual channel views show
original units with optional numeric **Values** controls. Equal start/end values
need a middle key to make a bounce. Use Animate (A) for direct canvas posing.

Timeline and Xsheet also support dragging pose diamonds. Enable **Keys** to add keys
with a double-click in an empty cell; turn it off for exposure-range selection and
new drawings. **+ Key** is always available. Timeline's **+ / −** buttons widen or
narrow the frame cells. Moving onto an occupied key or editing a locked layer rejects
the gesture without losing either pose.

With **Edit points**, double-click a pencil or polygon segment to insert a control
point, drag it to reshape the stroke, and use Delete/Backspace to remove the selected
point. The last pencil point and the final three polygon points are protected. Use
Select to delete the entire object. Rectangle/ellipse point counts are fixed.

**Clear** removes both exposures and animation keys within the selected range and
layers, regardless of the paste-content dropdown. The frame menu action clears one
cell and its key. Outside keys, rest transforms and drawing resources are preserved;
Undo restores the cleared exposures and keys together.

## Edit a block of pose keys

The **Keys** lane under either curve view now shares selection with Timeline and
Xsheet. Click a diamond, Shift-click to include the intervening keys, or Cmd/Ctrl-click
to toggle individual keys. Drag blank space in the curve Keys lane to select a span.
Clicking blank space in that lane clears selection; a selected diamond remains selected
when you begin a drag. Square curve keys continue to edit a single pose/channel.

Drag any selected diamond to move the whole group, retaining its spacing and curves.
**Alt-drag** duplicates it. With at least two keys selected, drag the slim **right edge**
of the highlighted band to stretch or compress timing around its first key. Rounding
that would merge keys, or a destination containing an unselected key, rejects the whole
operation. Escape cancels the preview. One Undo restores the complete edit.

**Copy** stores the selected poses; move to the desired frame/layer and **Paste** to
transfer the block. This copies local position, rotation, scale, opacity, pivots and
outgoing easing. It does not copy drawings or exposures, change the parent hierarchy,
convert between canvas sizes, or preserve a path in world space under a different
parent. Copy/Paste in Timing tools remains the separate exposure-range clipboard.

With the curve workspace or Timeline's **Keys** mode focused:

- Cmd/Ctrl+A selects all pose keys on the active layer.
- Cmd/Ctrl+C and Cmd/Ctrl+V copy/paste pose keys.
- Left/Right nudges the selected block by one frame; without selection it moves the playhead.
- Delete/Backspace removes the selected pose keys, preserving artwork and exposures.

Text fields keep their usual editing behavior. Changing layers clears key selection;
Undo restores document data and drops selected frames that no longer contain keys.
The in-memory motion clipboard can be reused after opening a different scene. These
operations use full-pose keys; independent channel timing remains pending.

## Edit motion positions on the canvas

Choose **Animate (A)**, expose a drawing and create pose keys. Enable **Path** in the
canvas toolbar to edit the evaluated trajectory directly. The transform box is hidden
in this mode so its handles do not compete with path markers.

- Click a round marker to go to that key's frame; the active marker shows its frame number.
- Drag a marker to move that pose's X/Y. Picking works within eleven screen pixels.
  Shift constrains movement to the dominant screen direction. Escape cancels; release
  creates one undo step. Rotation, scale, pivot, opacity, timing and easing are retained.
- Click near the path to scrub to a sampled frame. Double-click near it to add the
  evaluated pose at that integer frame. Existing keys are preserved. A new key starts
  with linear outgoing interpolation and can change the interpolation on either side.
- Turn **Path** off to return to the usual pose move/scale/rotation box.

The path tracks a fixed local reference: the drawing's bounds center when entering
Path. It includes animated parent transforms. The reference stays fixed when changing
frames; selecting another layer or re-entering Path captures a new center. Use key
navigation in Timeline/Curves when multiple markers overlap at the same position.

The gesture follows the cursor even through mirrored/rotated views and negative or
nonuniform parent scales. A zero-scale parent rejects movement; a zero-scale child
can still be repositioned. Changing the view, resizing, changing frames or changing
the document cancels an active preview. Locked layers cannot be edited.

This edits pose positions, not an independent spatial spline. Curves still controls
X/Y easing; separate path geometry and velocity, spatial Bezier handles and multi-key
path movement remain pending. Dense trajectories are sampled at integer frames.

## Compose and clean up vector drawings

Use **Lasso (L)** to surround whole strokes. Only fully enclosed footprints are
selected; hold Shift to add or Alt to subtract. Escape cancels. Select, Marquee and
Lasso preserve the same explicit vector group. **Edit → Select all / Invert selection**
works on vectors in the current drawing.

With the canvas focused, Cmd/Ctrl+C copies the selected vectors, Cmd/Ctrl+X cuts them,
and Cmd/Ctrl+V pastes them in place into the current drawing. Move the new selection
with its handles. The application clipboard also works after opening another scene;
it preserves colors and creates independent vector IDs. It does not exchange artwork
with other apps. Curve/timeline focus uses the separate motion or exposure clipboard;
text fields keep their normal shortcuts.

Use **Properties → Align** for edges/centers or horizontal/vertical distribution
(at least three vectors). **Order** moves selected strokes forward/backward or to the
front/back within their art layers. Arrow keys nudge one local pixel; Shift nudges ten.
Properties can apply a width, palette swatch, art layer or filled/outline state to the
whole selection. A mixed width displays **Mixed** until you enter a replacement.

**Pencil cleanup** offers Light/Medium/Strong smoothing and simplification tolerances
of 0.25, 0.5, 1, 2 or 4 local pixels. These affect sampled open pencil strokes, preserve
endpoints and pressure, and leave analytic primitives alone. Smoothing protects sharp
corners. Simplification bounds centerline and pressure changes, not the final painted
outline. Each action is one undo step; excessive work rejects without a partial edit.

Use **Line** for straight strokes. Hold Shift for 45-degree angles, or while drawing
Rectangle/Ellipse for squares/circles. **Guides** shows a drawing-local grid and enables
snapping for Pencil/Line/Rectangle/Ellipse. Constraints take priority over snapping.
Guides are session preferences; they are neither saved in the scene nor exported.

## Apply animation patterns

Select pose diamonds, then open **Curves → Keys** to apply Linear, Step, Smooth,
Ease in/out, Overshoot or Fast start / Soft stop to the selected keys. Easing presets
affect all outgoing pose channels and skip a final key with no following segment.
Use highlighted channels and round handles for further visual curve adjustments.

**Append 1 copy / Append 3 copies** repeats the selected sparse block after its last
frame, preserving all poses and easing. The period includes both selection endpoints.
Scene duration grows if needed and the new keys become selected. Existing keys are
never overwritten: any collision rejects the whole operation. Undo restores the
original block and duration. Matching start/end poses for a seamless loop remains
an artistic choice; repetition does not generate that transition automatically.
