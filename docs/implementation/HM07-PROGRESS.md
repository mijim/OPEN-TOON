# HM-07 named pose progress

HM-07 is in progress. Its first format-12 subset saves named poses on Character
roots. Each pose contains stable Part IDs, a per-Part mask over eight transform
channels and optional held drawing substitution, plus captured evaluated local
values. Applying a pose keys only included numeric channels at the playhead;
excluded values remain as evaluated there. A discrete substitution changes
only when its drawing bit is set. The operation validates a candidate and
creates one undo entry.

Properties now captures either the selected Part or all Parts with a visible
channel group. The panel selects, applies, renames and removes poses. Character
poses also have a direct blend slider: transform channels interpolate from the
pose at drag start, and the saved drawing wins at 50%. The stored endpoint is
exact; all drag updates coalesce into one undo entry. Character
duplication remaps pose/Part/drawing IDs into an independent copy. Branch
deletion and Part detachment prune affected entries. A referenced substitution
cannot be removed while a pose still needs it. Format 11 loads with no poses;
the first format-12 save preserves a source-version backup.
Rig can set or replace the selected Part's saved values and channel mask in
an existing pose, and remove a mapped Part while retaining at least one entry.
This makes per-Part masks editable after capture.
Rig also copies a selected pose to another compatible Character in the same
project. The command maps unique Part roles and unique drawing names, requires
matching local rest transforms, assigns new destination IDs and leaves the
source untouched. Incompatible mappings reject atomically. The copied pose
starts unpublished and a name collision gets a numeric suffix.
Rig's **Mirror** command creates a new pose by pairing `_left` and `_right`
Part roles. It reflects masked X, rotation and pivot-X deltas around each
Part's rest values, preserves the sign of other masked deltas, and maps an
included drawing by an unambiguous name. Unmapped or ambiguous pairs reject
without mutation. Unmasked channels retain destination rest values.

## Evidence

- `tests/rigging_tests.cpp`: mixed Part/channel masks, numeric blend endpoints,
  the 50% discrete drawing threshold, excluded channel preservation,
  single-step and coalesced undo/redo, independent copy and branch pruning.
  Per-Part refinement rejects an absent drawing and final-entry removal
  atomically.
- `tests/storage_tests.cpp`: format-12 JSON and SQLite round trip and format-11
  load without poses.
- `tests/export_tests.cpp`: actual editor imports original registered PNG
  Parts, assembles a character, captures a selected Part, applies at another
  frame, refines/removes a second Part mapping, blends with several live
  updates, undoes/redoes once, saves and reopens.
- Local macOS `build/locked`: 143/143 CTest entries pass. Native Qt Quick
  `--smoke-test` passes the mouse/input, window, deformation and reopen
  journeys. The new pose panel compiles and loads within that window; the
  controller journey exercises its backend rather than clicking its widgets.

## Published controls and workspaces

Format 13 lets a rigger publish a saved pose or coordinated view set. Rig
exposes their complete records and publication toggles; Animator shows only
published view buttons and a published pose picker with the 0–1 blend slider.
Both workspaces operate on the same document. The workspace mode is local Qt
view state, while publication is saved with each Character. Switching modes
keeps selection, frame and rendered output unchanged. Duplicating a Character
retains publication choices with new independent pose/view/Part IDs.

`tests/rigging_tests.cpp` checks publication and independent bindings without
render changes. `tests/storage_tests.cpp` checks format-12 loading with
publication off and a readable `.pre-v12.bak` after format-13 save.
`tests/export_tests.cpp` checks workspace selection/frame/pixel preservation
and save/reopen. The native `--hm07-smoke` opens the original continuous toon
project, publishes a view and pose, checks the visible QML dashboard and writes
`build/hm07-dashboard-smoke.png`. The screenshot was inspected at 2880 × 1832;
the connected character, view button and pose slider are visible.

Format 14 also lets Rig publish individual named drawing substitutions.
Animator groups the published choices by Part and switches only that Part at
the playhead. The domain keeps publication in independent Character copies;
format-13 projects load with publication off and their first format-14 save
creates a `.pre-v13.bak` source backup. The editor integration uses a
two-Part character to switch a published hand drawing, verify the other Part
and existing keys, undo/redo and reopen. The native smoke visibly renders
published mouth choices on the 15-Part continuous toon, switches one choice
and undoes it. The screenshot was inspected with the connected silhouette and
mouth picker visible.

Animator now also places a compact control panel in the camera viewport. Its
published pose slider and Part drawing pickers call the same bounded document
commands as the sidebar. It disappears in Rig and never enters output pixels.
The native mouse smoke checks the actual overlay, moves a mapped torso
position, verifies the mouth drawing, torso rotation, selection and frame are
unchanged, and restores the document with one undo. The visually inspected
`build/hm07-canvas-controls-smoke.png` shows the control panel beside the
connected character rather than over its silhouette.

Format 15 lets Rig assign each published view, pose or drawing substitution
to a named control group. The default `Main` preserves older projects.
Animator offers the available published groups in the side dashboard and
viewport panel; switching among Face, Body and Stage changes only which
controls are visible and applicable. It does not edit artwork, frame,
selection or rendered pixels. A hidden group's pose, view or drawing cannot
be applied through Animator controller actions. Independent Character copies
retain group names with new internal IDs. Format-14 loading defaults to Main;
the first format-15 save creates a readable `.pre-v14.bak` source backup.

`tests/rigging_tests.cpp` covers group validation, undo/redo, copied bindings
and unchanged render output. `tests/storage_tests.cpp` covers the format-14
migration and backup. `tests/export_tests.cpp` switches groups on an actual
imported Part and verifies hidden commands, save and reopen. The native smoke
shows Face's mouth control in both QML locations, Body's pose and Stage's
view-only state, then checks unchanged document/output. The visually inspected
`build/hm07-groups-smoke.png` shows the connected character with only the
Face control set.

`tests/rigging_tests.cpp` now transfers a pose across an independent character
copy, applies the mapped drawing and numeric channel, checks source isolation,
undo/redo and rejected mismatches. `tests/export_tests.cpp` checks the actual
editor's destination selection and save/reopen. The native smoke copies the
15-Part character, shows **Copy to** in Rig, transfers the pose and undoes it;
`build/hm07-transfer-smoke.png` was visually inspected. This is an in-project
compatible-rig subset, not an arbitrary rig retargeter.
The mirrored left/right Part fixture checks numeric reflection, drawing
mapping, excluded opacity, missing and ambiguous mappings, source isolation
and two-step undo. The native smoke checks the visible Rig mirror action and
its atomic undo on the 15-Part character.

The same native smoke drags the published slider across the continuous
15-Part character. Each run discards five warmup moves, measures 40 mouse
move-to-`frameSwapped` samples, verifies that a mapped Part moves and that one
undo restores the baseline. Three optimized macOS 15.5 / M1 Pro runs measured
p95 **20.28 / 19.01 / 17.83 ms** and peak process resident memory
**325.9 / 319.4 / 319.8 MB**. These sampled controls meet the proposed
50 ms and 2 GiB budgets on this host. The 48-frame scene excludes audio and
matte nodes; the complete 480-frame Harmony Moment remains unqualified for
control latency.

After adding the drawing picker, three further native runs measured p95
**18.48 / 18.10 / 18.09 ms** and peak process resident memory
**333.2 / 331.3 / 339.3 MB** with the same 40 samples and five warmups per
run. The published mouth command and one-step slider undo passed in each run.

With the floating viewport widget present, three native runs exercised its
mouse drag and undo before the same 40-sample panel timing loop. Panel
mouse-to-`frameSwapped` p95 was **18.45 / 17.91 / 18.05 ms** with peak process
resident memory **342.2 / 332.6 / 337.7 MB**. The overlay did not alter the
connected character or exceed the sampled 50 ms / 2 GiB budget on this host.

This remains a direct-command control subset. Persistent multi-driver
evaluation, conflict/cycle handling, general typed widget bindings,
broader cross-character mapping and a complete artist journey remain
open. HM-07 is still in progress.

## Remaining contract

Saved pose masks use explicit Part IDs within one character. Broader cross-rig
retargeting, broader mirroring, full published controls, general typed widgets,
conflict/cycle handling, complete workspace layout presets and artist
acceptance remain open. RIG-012 and HM-07 are partial.
