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

For a character rig in the current source build, open the
[continuous toon study](../../examples/clockwork-continuous.otoon) from
**Scene → Open**. Its four complete limb meshes, linked hands and feet,
face view change and hand substitution can be inspected across 48 frames.

**View → Composition** selects **Legacy appearance** or **Linear sRGB**. The first
keeps the established Qt scene appearance and is the default for old projects.
Linear sRGB composites drawing layers in linear light and can look brighter at
semi-transparent overlaps. The choice is saved with the project, is undoable,
and applies to canvas preview and PNG export. Current composition is 8-bit CPU
rendering. Unchanged linear frames are reused while you pan, zoom or rotate the
view; editing or switching scenes refreshes them. Open the bottom **Nodes**
tab to inspect the derived graph. Click an image or matte card to see its
output at the current frame; matte alpha appears as grayscale. The tab can
change drawing order, opacity, opacity bypass, Normal/Multiply/Screen/Add blend mode and cutter settings. Drag a Drawing card onto another to place it immediately above that target in composite order; the target card highlights during the drag. Locked drawings reject the move. Select a Drawing or Part,
choose a visible source in **Cutter matte**, then use **Outside**, **Bypass**
or **Paint cutter source** as needed. The last option keeps the source visible
at its normal layer order while it masks the selected target; it is shared by
other targets using that source. These edits undo and save. Arbitrary node
creation, wiring and grouping remain open.

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

The inspector edits position, rotation, scale, opacity and pivot. Add transform keys with Linear, Hold or Smooth interpolation. If a layer already has keys, editing a transform inserts or updates a key at the current frame. Parent layers apply inherited transforms and opacity. Character parts and pegs preserve the visible image when reparented within one character, provided the result has no shear or singular transform and the moved layer and the branches whose parentage changes are not animated. Motion above their shared ancestor is preserved. Curves can be edited in the main workspace. IK remains pending.

**Scene → Add output camera** creates one orthographic camera. Choose the Camera
tool to see the shot frame: drag inside it to pan, drag the round top handle to
rotate, or drag a corner to zoom. Shift constrains pan to one axis or rotation
to 15-degree increments. The compact camera bar and selected-camera Properties
show zoom and pose; **Reset** restores the scene-center frame. **Guides** shows
the frame and safe area over normal artwork editing. Camera moves at later
frames create keys and can be edited in Curves. Camera framing affects preview
and PNG export; viewport pan, zoom and fit only change how you inspect the shot.
The camera is single-output for now; multiplane and perspective are pending.

To assemble a rigid character, import registered PNG parts and select one layer in the layer list. In its Properties panel, open **Rig → Make character from layer**. That layer becomes a Part below a new Character root. Select another imported drawing layer and choose the Character or a Peg as its parent; the drawing becomes a Part while retaining its visible placement. **Rig → Add parent peg** inserts a transform parent over the selected Part or Peg. Edit the Part role in Properties. **Center rest pivot on drawing** places its saved pivot at the local artwork center while retaining its position; do this before animating the layer. Precise on-canvas pivot placement is pending.

**Rig → Attach unparented drawings** adds every exposed, unlocked root drawing to the selected character as a separate Part in one undoable step. It leaves unexposed guide layers alone and keeps registered artwork in place. In the layer menu, **Duplicate rig branch** copies a selected Part or Peg with its descendants and independent drawing identities; **Clone branch with linked artwork** keeps the same drawing resources while giving the new layers their own timing and view choices. **Rig → Delete selected rig branch** removes a Part or Peg subtree and its saved view choices. **Detach selected part** returns a Part to a root Drawing layer; **Dissolve peg, keep children** removes an unanimated Peg while preserving supported child rest poses. Animated or nonrepresentable cases report an error instead of changing the scene.

Each Part shows its named substitutions as thumbnail tiles in Properties. **+ Blank** starts a new drawing, **Duplicate** copies the currently exposed drawing, and clicking a tile chooses an option at the playhead through the current held interval. The chevrons step through options; arrows reorder them. Rename or remove the selected option in the same panel. A Character view set captures the current choice of every Part. Use **+ Capture**, **Apply** and **Update** to switch coordinated drawings at the playhead without changing transforms; view sets can also be renamed, duplicated or removed. **Rig → Duplicate full character** makes an independent hierarchy and artwork copy. These edits undo, save and reopen with the rest of the rig. View sets contain drawing choices only.

**Character poses** in Properties save a named pose at the current frame. Choose **Selected part** or **All parts**, then choose which transform channels or drawing substitutions to include. **+ Capture** records the evaluated pose; **Apply** keys only the chosen numeric channels and changes only included drawings at the playhead. Other channels keep their current values. The **Blend** slider moves from the current pose to the saved pose; a drawing change occurs at 50%. Its drag is one undo step. Rename or remove a pose in the same section. **Mirror** creates a new pose by pairing `_left` and `_right` Part roles, reflecting masked X/rotation/pivot-X offsets from rest and matching included drawings by name. Capture, application and deletion each undo in one step and survive save/reopen. A locked target rejects application. With another Character in the project, choose it beside **Copy to** to transfer the selected pose. Both Characters need unique matching Part roles, matching local rest transforms and unique matching names for included drawing choices. New mirrored or copied poses are unpublished; incompatible mappings leave the project unchanged. Broader rig retargeting, general control bindings and templates are still in progress.

To refine a saved pose, select a Part, choose its channel group, and press **Set this part**. This adds or replaces only that Part's captured entry at the current frame. **Remove part** removes its mapping when another Part remains. The entry list below the controls shows the mapped Parts and channel groups. These edits are undoable and reject an unexposed drawing or an empty final pose.

Choose **Rig** or **Animator** in the top workspace selector. In Rig, use **Show in Animator** on a saved view, pose or selected drawing substitution to publish it. Set its **Control group** to organize published controls; `Main` is the default. Animator shows one published group at a time: view buttons, drawing choices grouped by Part, and the selected published pose's Apply/Blend controls. A compact floating panel in the camera viewport repeats the current group's published pose slider and drawing choices so you can use them beside the character. Dragging its slider changes only the saved pose mask, with one undo per gesture; a drawing choice changes only its Part at the playhead. Group switching and the panel are view state and do not enter exported frames. Changing workspace preserves the selected layer and frame; it does not change output artwork. The workspace choice is stored locally for this computer. Publication and group names are saved in the project and follow an independent character copy with new bindings. General typed widgets, driver conflicts and broader rig retargeting remain in progress.

**Apply range** places the selected view across the selected half-open timeline range while preserving the exposures before and after it. **Set this part** updates just the selected Part's choice in the selected view from the current frame. Arrow controls beside the view selector step through or reorder view sets. Navigation changes selection only; applying a view is the explicit document edit.

## Mesh binding and animated controls

Select a Part with an exposed image-only or vector-only drawing. In its
Properties, choose a mesh grid from 1 to 32 columns and rows, then select
**Bind**. A tall continuous sleeve or trouser leg can use 6 × 16. The Mesh
tool appears with handles over that Part. **Pose vertices** lets you drag a handle to preview
the warp; **Rest vertices** edits the saved bind shape after resetting any
pose. **Reset pose** returns the handles to rest, and **Remove** returns the
Part to its unbound artwork. Each completed drag is one undo step. Switching
substitutions loads that drawing's own binding. Properties shows the saved grid
resolution of the selected substitution.

The vertex pose is a saved static preview that applies at every frame. Reset
it before choosing **Bone chain** or **Curve**. These bounded controls use the
selected drawing's mesh: drag a bone joint or curve point on canvas at the
current frame to record an animated pose. Escape cancels a drag. **Rest key**
records the original shape at the playhead without deleting other keys;
**Remove control** removes the deformer and leaves the rest mesh. One Part
substitution can have one control type; another substitution may use its own.
The mesh grid follows the rendered pose while a control moves. Static vertices
cannot be edited while a control is attached.

At the first frame of a substitution change, **Match previous pose** keys the
incoming bone or curve to the outgoing drawing's evaluated pose at that frame.
The button appears when both bindings use the same control type and identical
rest joints or curve controls. The edit is undoable and preserves an existing
key's interpolation. For a drawing with different rest geometry, place and
animate its controls directly.

Use **Rest joints** or **Rest curve** to place the saved control geometry on
canvas. **Pose joints** or **Pose curve** returns to animation editing.
**Elbow influence (px)** adjusts how far the bend blends into either segment;
the existing keys and rest artwork stay in place. A value that folds the mesh
is rejected. In **Rest joints**, the dashed circle and square handle at the
elbow show the same radius. Drag the square to tune it on canvas; Escape
cancels. To avoid a visible cut at the elbow or knee, use one continuous
image and mesh for the whole limb. Hands and feet may remain separate for
substitutions.

To keep a hand or foot attached while its one-piece limb bends, select the
hand/foot Part. In **Parent**, choose the bone-bound limb Part, then choose
**Rig → Follow parent bone tip**. The link is saved and undoable; the child's
drawing and substitutions remain independent. The source limb needs a bound
two-segment bone on every exposed drawing and rest at frame zero. Attach before
adding transform keys to the child. If a source substitution uses a different
rest wrist or ankle, the child follows that variant's evaluated endpoint and
direction, even when that variant is later selected at frame zero. The same
Rig action detaches it; detach
before reparenting. Curve attachments, IK and automatic limb setup remain open.

Insert/remove frames and Clear include bone/curve keys. Copy/paste, move and
stretch of a range within the same Part transfer its substitution keys.
Cross-scene and different-Part deformer-key paste report that a portable rig
copy is needed; the source range remains intact. Independent drawing paste
also rejects a bound Part until its mesh can be cloned with the drawing.

Image grids start at visible alpha bounds; vector drawings
remain editable and use a scene-resolution raster proxy. Raster-tile and mixed
media drawings cannot bind yet. Mesh proxies are limited to 4096 pixels per
axis. If a binding no longer matches its source, the edit is rejected; remove
the binding before deleting its substitution or detaching the Part. The
animated control profile is still being qualified for extreme bends and the
complete character shot.

## Saving and recovery

Each manual save appends a complete revision. Scene history can restore a saved revision as an undoable edit. A stale writer is rejected if another writer changed the revision on disk; save a separate copy or reopen to resolve it.

Every 60 seconds, a modified document is saved to a recovery file. On a later launch, recovery can open that snapshot as an unsaved scene. Save it under a chosen name. Autosave runs from an immutable snapshot in a worker, so newer edits stay unsaved until the next save. Manual saves remain synchronous. Recovery from operating-system power loss and multiple simultaneous application instances has not been qualified.

Format 2 compresses and shares media between revisions. The metadata limit is 64 MiB and decoded image media is limited to 512 MiB. Undo snapshots share unchanged media; vector metadata is copied. Opening formats 1–27 remains supported. Format 3 added channel Bézier easing, format 4 added typed character layers and named substitutions, format 5 added character view sets, format 6 saves the composition profile, format 7 added the output camera, format 8 saves mesh bindings, format 9 saves bone/curve controls and animation keys, format 10 introduced parent bone-tip links, format 11 saves their rest tip and distal direction at attachment time, format 12 adds named masked character poses, format 13 adds publication flags for character poses and views, format 14 adds publication flags for drawing substitutions, format 15 adds published control groups, format 16 adds PCM16 WAV assets and placed clips, format 17 adds nondestructive clip repeats, format 18 adds cutter bindings, format 19 adds Outside coverage, format 20 adds cutter bypass, format 21 adds optional cutter-source painting, format 22 adds source-sample clip fades, format 23 adds per-clip mute, format 24 adds Drawing/Part opacity bypass, and format 25 adds Normal/Multiply/Screen layer blending, format 26 adds per-clip solo, format 27 adds per-clip stereo balance, and format 28 adds Add layer blending. The first save of an older schema creates a backup named for its source version, such as `.pre-v20.bak`, before upgrading. Older editors require that backup to reopen the original format.

**Scene → Compact project history** retains the chosen number of newest saved revisions and removes unreachable media, after creating a full `.pre-compact.bak` backup. Save pending edits first. This is an explicit operation and is not part of autosave.

## Timeline ranges

Drag over cells to select a rectangular frame/layer range. **Edit → Timeline range** provides copy, paste (linked exposures, independent drawings, keys, or all), insertion, repeats, stretch and timing on ones/twos/threes. Copy/Paste shortcuts work when the timeline has focus. Paste starts at the playhead and can extend the scene. Copy a range, open another scene in the same window and paste to create independent drawings with remapped palette IDs. The clipboard is local to that window.

Alt-drag inside a selection to move it; the shaded destination shows the overwrite range. Escape cancels a pending move. Stretch refuses any compression that would remove a drawing or merge keys. Timing on ones/twos/threes removes gaps within the selection and overwrites the resulting destination span. Locked selected layers reject the whole edit.

Use **Edit → Scene marker** to label the current frame; an empty name removes its marker. **Export Xsheet PDF** writes a paginated sheet, limited to 200 pages to keep the synchronous operation bounded.

## Export and limitations

PNG export evaluates an immutable snapshot, so later edits do not change the running export. Cancel stops between frames. `manifest.json` records the rational frame rate, frame count and completion/cancellation/failure state. A cancelled or failed directory contains partial output and must not be treated as a complete sequence.

**Scene → Import image** loads one image up to 4096 × 4096. Tagged images are
converted to sRGB; untagged colors are interpreted as sRGB. If the scene has no
layers, creating the layer and importing its drawing are one undoable action.
For character artwork,
**Import registered PNG parts** creates one layer per selected file; every PNG must
have an alpha channel and the same canvas size. Their shared canvas origin is
preserved, with no automatic crop or orientation transform. Parts are ordered by
filename. Transparent PNGs with a non-sRGB color profile are rejected; untagged
PNGs are interpreted as sRGB and reported in the status bar.

**Import PNG sequence** creates one layer with a separate one-frame drawing for each
numbered file. Files need a common prefix and number width, such as `walk_0001.png`
and `walk_0003.png`. Import begins at the current frame; missing numbers leave empty
frames, which the status bar counts. Both batch modes have a 4096 × 4096 pixel/file
limit, a 256 MiB decoded-media limit and an undoable all-or-nothing commit. Cancel
the file chooser to leave the document untouched. The project format also enforces
its overall scene media limit. Parts currently share a centered layer offset; there
is no automatic character-role assignment; use the Rig controls after import. Layered PSD, audio playback, video
output, lip sync, animated deformers, node effects, OCIO, reusable rig libraries and
production installers remain pending. Consult the [phase status](STATUS.md).

## Audio timing subset

Choose **Scene → Import PCM16 WAV** to place a local mono or stereo 16-bit PCM
file at the current frame. A separate row below the drawing layers shows its
waveform; its exact peak index follows the timeline's frame width. Drag the
waveform to move its clip by whole frames; release commits one undo step and
Escape cancels. The Audio section in the right panel edits the clip's start
frame, source in/out samples, linear gain and a repeat count from 1 to 64.
Enter both sample endpoints and choose **Set** to trim. **Duplicate** creates
another placement at the playhead with the same trim, gain, repeats and fades;
both clips share the embedded source. **Remove clip**
removes its placement while leaving the source available to its other copies.
For a 48 kHz single-pass clip, drag the small middle handle at either waveform
edge to trim by whole scene frames. The left handle keeps source time aligned
while moving the clip start; the right handle changes the source out-sample.
Escape cancels a drag. Other source rates and repeated clips use the numeric
sample fields.
**Split** at an interior playhead frame divides one 48 kHz, single-pass clip
into two placements sharing the same WAV. The cut must leave samples on both
sides and stay outside the fade intervals. At 48 kHz output the before/after
mix is sample-identical; other source rates and repeated clips currently
report an error. Playback at another device rate may differ near the cut.
**Mute** removes that clip from playback, scrub and WAV export while keeping
the placement and waveform visible in a dim state; **Unmute** restores it.
**Solo** isolates that clip with any other soloed clips in playback, scrub and
WAV export. Mute still silences a soloed clip. **Unsolo** restores the shared
mix. Clips excluded by solo remain visible with dim waveforms.
**Balance** ranges from -1 (left) through 0 (center) to +1 (right). It
attenuates the opposite output channel without changing the original WAV;
preview and WAV export use the saved setting.
**Linear fades · source samples** sets fade-in and fade-out lengths across
the whole repeated clip; a repeat boundary does not restart the envelope.
The waveform shows source peaks and gain with fade guides over the clip;
preview and WAV export apply the fades. Shortening a trim or repeat count
clamps fades to fit. Drag the small upper handle at either guide to adjust
that fade directly; Escape cancels and release makes one undoable edit.
Import, duplication, split, mute, solo, move, trim, gain and removal are undoable and survive
save/reopen.

The format accepts up to 128 MiB per WAV, 512 MiB total audio, 64 assets and 1,000 clips. Clips
start on whole scene frames, and source-sample trim uses a half-open interval.
Repeats join the trimmed source interval in sample time without editing the
original WAV.
Unsupported or damaged files leave the project unchanged. **Scene → Export PCM
WAV mix** writes a 48 kHz stereo mix from all placed clips. It runs in the
background and can be cancelled without replacing an existing destination.
**Play** previews the placed clips through the output device, follows its
submitted sample cursor, and loops at the scene end. Clicking or dragging the
playhead while playing seeks the audio. With playback stopped, press and drag
across the timeline ruler or drawing rows to hear short fragments at each
frame; release stops the sound. Editing stops playback so the next
preview uses the new document. If the device cannot open, the visual preview
continues silently and shows an error. Hardware scrub quality and device-loss
recovery are still open. [HM-10 progress](HM10-PROGRESS.md) records
the current test evidence and remaining work.

## Animation edits and curves

Choose **Setup** in Properties to change rest values without creating keys. Existing
keys retain their poses; the canvas continues to display the evaluated animation.
Choose **Animate** to edit the current key. Enable **Auto key** to create a full-pose
key when editing an unkeyed frame, or press **Add key** explicitly. With Auto key off,
unkeyed edits are rejected. The inspector labels keyed, interpolated and held poses.

The selected layer's **Pose** menu has ten actions: copy its current transform;
paste all transform channels, only position, rotation, scale, opacity or pivot;
paste a horizontally or vertically mirrored transform; and reset. Copy takes the
rest transform in Setup or the evaluated transform at the playhead in Animate.
Pasting in Setup edits the target layer's rest transform and leaves its existing
keys alone. Pasting in Animate explicitly creates or updates one full-pose key,
even with Auto key off; a first later-frame paste anchors the unchanged rest pose
at frame zero. Mirrored paste negates the corresponding scale around the target
pivot. Reset means identity transform in Setup, or the target layer's rest pose in
Animate. Each paste/reset is one undo step. The copied transform stays available
when changing scenes in the same window; it contains values, not artwork or rig
references. Locked layers reject paste/reset.

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
