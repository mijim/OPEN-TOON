# ADR-013 — Explicit animation edits and pose curves

Accepted for the experimental editor, 2026-09-20. Owner: `modules/document/animation`;
Qt adapter: `adapters/qt/animation_controller.cpp`.

Setup edits only the static `Layer::transform`; it never inserts or changes keys.
The inspector shows rest values in Setup. Existing full-pose keys override the rest
transform during evaluation, including endpoint holds. Thus changing rest values on
an animated layer does not offset its animation. The viewport continues to show the
evaluated frame; this is not yet a rig rest-pose preview mode.

Animate edits an existing key at the playhead. On an unkeyed frame it requires Auto
key; otherwise it rejects the edit with an English status message. Auto key samples
the current evaluated pose before changing the requested field. Add key explicitly
switches to Animate. Modes and Auto key are session UI state, not project data.

Keys continue to store all eight transform values together. The curve editor edits
one value without changing other values, but moving a key or changing its outgoing
interpolation affects the full pose. This is not independent channel keying.
Positions/pivots are scene pixels, rotation is unwrapped degrees, scale is a signed
ratio and opacity is 0–1. Linear, Hold and Smooth retain the existing evaluator:
Smooth uses `t*t*(3-2*t)` with fixed zero endpoint slopes. Extrapolation holds the
nearest endpoint. No editable Bezier tangents or spatial path/speed separation yet.
Graph samples use the same integer-frame evaluator as preview and export, with a
bounded regular sample grid plus key and preceding-frame samples. The graph joins
these samples; it is not a subframe evaluation API.

Key-only retiming accepts half-open source selections, and maps first/last source
frame positions to first/last destination frame positions using nearest-integer
rounding. A one-frame selection maps to the destination start. Any collision with
another selected or unselected key rejects the whole operation. It may extend scene
duration; it never moves drawings, exposures, markers or unselected keys. All layers
are checked before publishing updates. UI edits are transactional Session commands.
One drag commits once on release; Escape cancels its preview. Numeric controls offer
an alternative to dragging. No serialization change or new dependency is required.
