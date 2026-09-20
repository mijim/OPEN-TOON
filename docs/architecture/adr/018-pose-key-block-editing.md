# ADR-018 — Direct pose-key block editing

Accepted for the experimental P06 subset, 2026-09-20. Feature owners: ANI-009
(multi-key editing) and ANI-010 (copy motion). Modules: `document/key_block`, the Qt
key-selection adapter and the reusable QML `PoseKeyStrip`.

## Scope and contract

Keys still contain a complete local-layer pose. Selection is a sorted set of actual
key frames on one layer, shared by Curves, Timeline and Xsheet. It is transient UI
state: it does not dirty a project, enter its format or create history entries.
Changing layers/scenes clears selection; document/history changes prune missing keys.
Undo restores document data, not the previous UI selection. Individual channel times,
multiple-layer selection and arbitrary channel masks remain outside this subset.

Shift-click selects the anchored span; Command/Control-click toggles a key. Dragging
blank space in the curve Keys lane selects a span. Drag selected diamonds to move the
block; Alt-drag duplicates it. Drag the selection's right edge to stretch from its
fixed first key, with nearest-frame rounding of each relative time. Shapes, values,
base interpolation and normalized outgoing easing remain unchanged. Exposures,
drawings, markers, static transforms and unrelated keys are not retimed.

All destinations are prepared and checked before publication. Collisions with an
unselected key, between rounded selected keys, or with originals during duplication
reject the entire operation with an English explanation. Locks and the existing
one-million-frame bound apply. Pointer previews stay within the scene; keyboard
nudges and pasted blocks can extend its duration. Escape/ungrab/document changes
cancel preview. Release commits once and Undo restores the entire block.

## Motion transfer

The internal pose clipboard stores only keyframes, with time zero at the first
selected key. Copy does not require an unlocked source. Paste starts at the current
frame of the active target layer; no drawing, exposure, hierarchy or rest transform
is transferred. Position and pivot numbers remain local scene pixels, rotation is
unwrapped degrees, signed scale is a ratio, opacity is 0–1. Pivots are copied as part
of the pose. Parent transforms, different canvas sizes and pivots are **not** converted
to preserve world-space paths. Toolbar hints and status messages state local units.
The clipboard can survive scene changes because it contains no resource IDs.

Curves exposes compact Copy/Paste controls. Copy, Paste, Select All, Delete/Backspace
and arrow nudges act on pose selection only when the curve workspace or Timeline's
Keys mode has focus, and never while editing text. In exposure mode, Timeline keeps
its separate exposure clipboard. Square curve keys continue to edit one pose/value;
the diamond lane owns batch timing. No serialization or dependency change is required.

## Acceptance and evidence

- Sparse/noncontiguous selection leaves unselected keys and artwork untouched.
- Stretching a block preserves evaluated motion at exactly mapped sample times.
- Collisions, invalid spans and locked targets leave the document unchanged.
- Duplicate/delete/transfer preserve poses, easing, pivots and parent ownership;
  save/reopen and rendered output remain consistent.
- Native mouse/keyboard workflow checks cover span/toggle/box selection, group drag,
  stretch, duplication, collision, cancellation, focus-safe Delete, copy/paste,
  shared Timeline selection and one-step Undo.

P06 remains partial. Cross-layer world-motion retargeting, independent channel keys,
cameras, spatial paths and velocity separation require further work.
