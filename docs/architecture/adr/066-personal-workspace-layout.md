# ADR-066 — Personal lower-workspace layout

Status: adopted for the experimental desktop editor, 2026-09-30.

The lower editor tab, splitter height, timeline cell width and Timing tools
visibility are personal workspace preferences. Keep them in Qt `QSettings`
alongside the existing Rig/Animator mode. The scene file owns drawings,
timing and character data; temporary splitter drag preview stays in QML until
release. A layout change must not create a scene revision or modify selection
and current frame.

Validate settings at the adapter boundary: accept only known tab names,
clamp stored dimensions to supported ranges and use defaults for malformed
numbers. The View menu offers one reset action. Keep the resolved layout
bounded by the current window size without overwriting the stored preferred
height during a resize.

Controller reopen/reset tests and native Qt Quick clicks and drag provide
evidence for this subset. Full dockable workspace presets, keyboard shortcut
configuration, UI scale and complete preference separation remain open under
UI-002 and UI-005; this decision does not claim their acceptance.
