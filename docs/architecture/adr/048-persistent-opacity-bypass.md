# ADR-048 — Persistent layer opacity bypass

Status: experimental HM-12 subset, 2026-09-29.

## Decision

Format 24 adds `opacityBypassed` to Drawing and Part layers. Formats 1–23 load
with the flag off; the first save of an older project keeps a readable
source-version backup. Bypass is a transactional command with undo and redo.
It keeps setup opacity and all opacity keys intact. Other layer kinds reject
the flag.

The derived graph replaces an active `Opacity` node with `BypassOpacity` when
the flag is on. The bypass node forwards its input image without multiplying
alpha. A cutter reads the same forwarded image, so fractional coverage and
optional painted source agree in Display and Write. Toggling bypass never
changes drawing art or animation values. The Nodes toolbar exposes the flag
for the selected Drawing or Part.
The reference renderer enters the graph for a bypassed layer even in a legacy
scene without a cutter. Its isolated source copy clears the bypass flag to
avoid re-entering the graph while rendering that source.

## Evidence and limits

Render tests exercise keyed target and cutter-source opacity, Inside matte,
both terminal outputs, the legacy scene path without a matte, undo/redo and
save/reopen. Storage tests migrate a
format-23 project, verify its readable backup and reject a malformed
format-24 layer. Native Qt Quick smoke clicks the control, checks pixels,
undo/redo and reopens the saved scene. This is a layer-local node operation;
arbitrary graph editing and general node bypass remain HM-12 work.
