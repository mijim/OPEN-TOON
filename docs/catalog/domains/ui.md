# UI — Workspace, navigation and preferences

[Back to catalog](../README.md)

> Generated from `features.json` and `domains.json`; do not edit manually.

**Workflow:** Choose workspace → draw or animate → customize panels and shortcuts.

**Module:** `workspace`.

**Entities:** Workspace, PanelLayout, ShortcutMap, ToolPreset.

**Relationships:** PRJ.

**Main risk:** Professional density versus readability; ambiguous tool states.

## Shared contract

Mutations must respect transactions, undo/redo and persistence. Visual aids are not exported. Errors, cancellation and unsupported data must preserve the last valid state. These OPEN-TOON conditions will be specified per operation during implementation.

## UI-001 — Dockable panels

Move, split, group and reset panels without losing the document.

**Initial acceptance:** Restoring a layout leaves every view accessible.

**Scope:** `base` · **Level:** `core` · **Status:** `not_started`.

**Evidence:** `proposal`.

## UI-002 — Saved workspaces

Save and switch layouts for drawing, animation, rigging and compositing.

**Initial acceptance:** Switching workspaces preserves the selection and current frame.

**Scope:** `base` · **Level:** `pro` · **Status:** `not_started`.

**Evidence:** `proposal`.

## UI-003 — Customizable toolbars

Show relevant tools and allow frequently used actions to be reordered.

**Initial acceptance:** A custom button continues to work after restarting.

**Scope:** `base` · **Level:** `pro` · **Status:** `not_started`.

**Evidence:** `proposal`.

## UI-004 — Configurable shortcuts

Assign contextual shortcuts with conflict detection and command search.

**Initial acceptance:** A shortcut inside a text field does not trigger a destructive action.

**Scope:** `base` · **Level:** `core` · **Status:** `not_started`.

**Evidence:** `proposal`.

## UI-005 — Persistent preferences

Separate personal preferences, document settings and temporary values.

**Initial acceptance:** Changing UI scale does not modify the project file.

**Scope:** `base` · **Level:** `core` · **Status:** `not_started`.

**Evidence:** `proposal`.

## UI-006 — View zoom, pan and rotation

Navigate the canvas without changing geometry or keyframes.

**Initial acceptance:** Rotating the view by 90 degrees does not change the exported render.

**Scope:** `base` · **Level:** `core` · **Status:** `not_started`.

**Evidence:** `proposal`.

## UI-007 — Mirror view

Mirror the displayed drawing for inspection without reflecting stored data.

**Initial acceptance:** Saving with a mirrored view does not reverse the exported image.

**Scope:** `base` · **Level:** `core` · **Status:** `not_started`.

**Evidence:** `proposal`.

## UI-008 — Camera and drawing views

Distinguish isolated drawing editing from the composited scene result.

**Initial acceptance:** The selected drawing can be edited without baking effects into its data.

**Scope:** `base` · **Level:** `core` · **Status:** `not_started`.

**Evidence:** `proposal`.

## UI-009 — Contextual inspector

Show editable attributes, animation state and units for the current selection.

**Initial acceptance:** Multi-object editing displays mixed values correctly.

**Scope:** `base` · **Level:** `core` · **Status:** `not_started`.

**Evidence:** `proposal`.

## UI-010 — Tablet input

Use pressure and sensitivity settings, with visible fallback when the device lacks support.

**Initial acceptance:** The same gesture varies stroke width according to the selected pressure curve.

**Scope:** `base` · **Level:** `core` · **Status:** `not_started`.

**Evidence:** `proposal`.
