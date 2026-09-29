# ADR-057 — Alternate Display source for node inspection

Status: experimental HM-12 subset, 2026-09-29.

## Decision

The Nodes preview may route one typed image or matte node to the canvas Display without
changing the document graph or Write. `CanvasItem` owns this selection as view
state: node ID, kind, layer owner and scene generation. It renders that node's
dependency closure through the existing graph evaluator. Transform outputs
cannot be selected. Mattes show fractional alpha as opaque grayscale. The **Show final output**
action returns to the ordinary scene preview.

A document edit that changes the selected node's identity clears the alternate
Display. Frame changes keep it while identity remains valid. Working drawing
previews and pose previews use the ordinary scene while the gesture is active.
Write, saved project state, undo/redo and export always use the final terminal.
This adds no project-format field or second evaluator.

## Evidence and limits

Native Qt Quick smoke selects a fractional blue Drawing source while Write
contains the red cutter target. It checks opposite canvas pixel colors, the
unchanged document revision and final output alpha, restores the final canvas,
then reorders the layers and checks that the stale Display selection clears.
It also rejects missing and transform node requests without changing Display.
The locked macOS CTest suite and native HM-12 smoke pass.

Display selection evaluates at canvas resolution through the bounded
node-scoped revision cache described in [ADR-058](058-node-scoped-display-cache.md).
Arbitrary graph wiring, multiple Write outputs and
published group ports remain open.
