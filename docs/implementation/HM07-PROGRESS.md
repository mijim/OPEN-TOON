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

## Evidence

- `tests/rigging_tests.cpp`: mixed Part/channel masks, numeric blend endpoints,
  the 50% discrete drawing threshold, excluded channel preservation,
  single-step and coalesced undo/redo, independent copy and branch pruning.
- `tests/storage_tests.cpp`: format-12 JSON and SQLite round trip and format-11
  load without poses.
- `tests/export_tests.cpp`: actual editor imports original registered PNG
  Parts, assembles a character, captures a selected Part, applies at another
  frame, blends with several live updates, undoes/redoes once, saves and reopens.
- Local macOS `build/locked`: 137/137 CTest entries pass. Native Qt Quick
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

This remains a direct-command control subset. Persistent multi-driver
evaluation, conflict/cycle handling, on-canvas widgets, control groups,
cross-character mapping and a complete artist journey remain open. HM-07 is
still in progress.

## Remaining contract

Saved pose masks use explicit Part IDs within one character. Cross-rig stable
role mapping, mirroring, full published controls, direct widgets, control
groups, conflict/cycle handling, complete workspace layout presets and artist
acceptance remain open. RIG-012 and HM-07 are partial.
