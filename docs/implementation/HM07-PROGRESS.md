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
- Local macOS `build/locked`: 135/135 CTest entries pass. Native Qt Quick
  `--smoke-test` passes the mouse/input, window, deformation and reopen
  journeys. The new pose panel compiles and loads within that window; the
  controller journey exercises its backend rather than clicking its widgets.

## Remaining contract

Saved pose masks support explicit Part IDs within one character. Cross-rig
stable role mappings, mirroring, published controls/switches, dashboard and
Animator/Rig workspace
views remain open. RIG-012 and HM-07 are partial; the recorded tests do not
claim artist acceptance of the complete control workflow.
