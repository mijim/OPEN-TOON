# ADR-067 — Named personal workspace layouts

Status: adopted for the experimental desktop editor, 2026-09-30.

Expose Drawing, Animation, Rigging and Compositing as named starting layouts
for the existing Rig/Animator mode and lower Timeline/Xsheet/Curves/Nodes
workspace. Each name has a factory layout and an optional personal override.
The View menu can save the current controls over a name; applying a name
loads its override or factory values. Manual changes label the active layout
Custom. The settings live in Qt `QSettings` under `layout/presets/<name>`;
the document format and its revisions remain unchanged.

Only known names, modes and tabs are accepted. Saved dimensions are bounded
before application. Switching may update presentation mode but must preserve
the selected document target and current frame. Changing the window size
temporarily bounds the visible lower panel without rewriting its saved height.

This decision covers named layouts of controls the current editor actually
has. Arbitrary dock placement, hidden panel sets, keyboard shortcut sets and
full workspace serialization remain open under UI-002. Controller reopen and
native Qt Quick checks exercise the available subset.
