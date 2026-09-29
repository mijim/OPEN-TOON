# ADR-069 — Resizable inspector in personal layouts

Status: adopted for the experimental desktop editor, 2026-09-30.

The Properties/Character panel has a visible horizontal splitter beside the
canvas. Dragging previews a width from 250 to 520 device-independent pixels;
release commits it to personal Qt `QSettings`, and double-click restores 250.
The current window bounds the visible panel so at least 360 pixels remain for
the canvas, without rewriting the preferred width. The inspector's content
uses the actual available width rather than the old fixed 248-pixel column.

The preferred width joins the Drawing, Animation, Rigging and Compositing
layout snapshots. Older personal settings without that key use each factory
layout's width. Manual resizing marks a named layout Custom until it is saved
again. Selection, current frame, document revision and project bytes do not
change. No project format change is needed.

Controller tests cover preferences, named overrides, malformed values and
reopen. Native Qt Quick pointer input drags the splitter and checks the saved
width, reset and profile switching. Full dock placement and panel visibility
presets remain open under UI-002.
