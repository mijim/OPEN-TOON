# ADR-017 — Combined motion overview and direct point operations

Status: accepted for the experimental P04/P06 subset, 2026-09-20.

## Combined animation by default

Curves opens on **All motion**. The overview displays every changing local transform
channel of the selected layer, sampled through the shared evaluator. Each curve is
normalized independently to a fitted range including its samples and control points,
with padding so overshoot handles remain reachable. The 0–100% scale is relative to
that display range, not a physical magnitude or a sum of pixels, degrees and opacity.
Constant channels are omitted. The canvas continues to show inherited world movement;
the graph does not flatten the parent hierarchy.

The compact legend highlights a channel **inside All motion**, leaving the others
visible. Its square keys drag in time and value; round handles shape its outgoing
transition. Clicking a curve selects its channel and preceding key. **Link easing**
is enabled initially: a handle drag writes the same normalized timing controls to
all eight pose channels in one command. Disabling it edits only the highlighted
channel. Previews do not mutate the document. Each committed gesture has one undo;
Escape, pointer cancellation or a document change abandons the draft.

Full-pose diamonds occupy a separate key lane. Drag horizontally to retime every
channel without changing values or outgoing easing. The same diamonds can be dragged
in Timeline and Xsheet; Keys mode makes blank-cell double-clicks create pose keys.
Collision/lock errors reject the complete operation rather than overwriting a pose.
In a graph, double-clicking adds a key with the clicked active-channel value and the
other evaluated values. In the Keys lane, it records the evaluated complete pose.

In All motion, Linear/Ease/Overshoot affect every outgoing channel of the selected
pose in a single command, independently of Link easing (which governs manual handles).
The dropdown retains individual channels in original units, with optional numeric
editing. Time zoom centers on the current frame; a scrollbar pans the visible window
and Fit restores the full duration and value bounds. Timeline cell-width controls and
larger screen-space key targets aid precise placement without enlarging the toolbar.

Owners: `ui/components/MotionOverview.qml`, the Qt animation controller and the
Qt-free `animation` module. No format or dependency change; format 3 already stores
per-channel outgoing easing.

## Sampled vector point editing

Edit points supports double-click insertion on a pencil/polyline or polygon segment.
Picking/projection uses screen coordinates and an eight-pixel tolerance, including
the closing polygon segment. Insertion interpolates position and pressure along the
segment and retains stroke identity, swatch, width and art layer. Delete/Backspace
removes the selected point, not the entire object. A pencil must retain one point;
a polygon must retain three. Rectangle/ellipse primitives reject point insertion
and deletion. These operations do not implement analytic Bézier contour tangents.

Each operation is transactional and undoable; locked layers reject edits. Point
insertion can refine the current renderer's sampled pressure-width approximation;
pixel identity after inserting a pressure-varying sample is not promised.

Owners: `drawing_selection` domain operations and Qt controller/canvas input.

## Clear semantics

The workspace **Clear** button always clears exposures and transform keys in the
selected half-open range and selected layers, independently of paste mode. Keys and
exposures outside that range remain unchanged; unexposed drawing resources are kept.
The frame menu action similarly clears the current exposure cell and a key exactly
at that frame. Rest transforms are not reset. Undo restores exposures and keys together.
An explicitly requested exposure-only domain operation remains available internally.

Verification covers normalized overview population, native pose-diamond dragging in all three views, linked/unlinked handles, square-key
editing, key insertion, time zoom,
point insertion/deletion and undo, and the actual Clear button in its default paste
mode. Domain/controller tests cover pose preservation, collisions, shape minimums,
pressure interpolation, range boundaries and locked-layer rejection. P04/P06 remain
partial; cameras, analytic contour topology, independent channel times, spatial
velocity and drawing morphing are still open.
