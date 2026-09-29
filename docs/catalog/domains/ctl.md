# CTL — Character controllers and dashboards

[Back to catalog](../README.md)

> Generated from `features.json` and `domains.json`; do not edit manually.

**Workflow:** Capture poses → define controls → interpolate → package.

**Module:** `controllers`.

**Entities:** Controller, PoseSample, ControlBinding, ControllerWidget.

**Relationships:** DEF, ANI.

**Main risk:** Pose interpolation does not invent drawings or resolve incompatible topology.

## Shared contract

Mutations must respect transactions, undo/redo and persistence. Visual aids are not exported. Errors, cancellation and unsupported data must preserve the last valid state. These OPEN-TOON conditions will be specified per operation during implementation.

## CTL-001 — Camera-view controls

Show selectable widgets bound to rig attributes with limits.

**Initial acceptance:** Dragging a widget changes only its linked attributes.

**Scope:** `base` · **Level:** `advanced` · **Status:** `partial`.

**Specification source:** `proposal`.

**Implementation evidence:** [docs/implementation/HM07-PROGRESS.md](../../../docs/implementation/HM07-PROGRESS.md) — bounded viewport pose slider and published Part drawing choices, general typed bindings and control placement remain open.

**Remaining scope:** The proposed behavior and acceptance test are OPEN-TOON requirements, not a claim of implementation.

## CTL-002 — Pose slider

Generate a one-dimensional control from compatible poses.

**Initial acceptance:** At each sample position, the controller reproduces the captured pose exactly.

**Scope:** `base` · **Level:** `advanced` · **Status:** `partial`.

**Specification source:** `proposal`.

**Implementation evidence:** [docs/implementation/HM07-PROGRESS.md](../../../docs/implementation/HM07-PROGRESS.md) — bounded published pose/view dashboard subset, full catalog acceptance open.

**Remaining scope:** The proposed behavior and acceptance test are OPEN-TOON requirements, not a claim of implementation.

## CTL-003 — Pose grid

Create a two-dimensional control over organized pose samples.

**Initial acceptance:** Grid points reproduce their samples and cells interpolate between them.

**Scope:** `base` · **Level:** `advanced` · **Status:** `not_started`.

**Specification source:** `proposal`.

## CTL-004 — Function wizard

Connect controls to attributes, activation and visibility without writing every script.

**Initial acceptance:** A switch can show a control set without altering the render.

**Scope:** `base` · **Level:** `advanced` · **Status:** `partial`.

**Specification source:** `proposal`.

**Implementation evidence:** [docs/implementation/HM07-PROGRESS.md](../../../docs/implementation/HM07-PROGRESS.md) — named groups filter published pose/view/drawing controls in Animator, general binding wizard remains open.

**Remaining scope:** The proposed behavior and acceptance test are OPEN-TOON requirements, not a claim of implementation.

## CTL-005 — Character dashboard

Group identifiable controls for character manipulation.

**Initial acceptance:** Duplicating the character creates independent bindings.

**Scope:** `base` · **Level:** `advanced` · **Status:** `partial`.

**Specification source:** `proposal`.

**Implementation evidence:** [docs/implementation/HM07-PROGRESS.md](../../../docs/implementation/HM07-PROGRESS.md) — grouped published pose/view/drawing dashboard and compact viewport controls, full catalog acceptance open.

**Remaining scope:** The proposed behavior and acceptance test are OPEN-TOON requirements, not a claim of implementation.

## CTL-006 — Scripted controller

Expose events and attributes for validated custom controls.

**Initial acceptance:** Script errors are reported without corrupting the scene.

**Scope:** `base` · **Level:** `advanced` · **Status:** `not_started`.

**Specification source:** `proposal`.

## CTL-007 — Deformer on deformer

Compose controls acting on another deformation structure.

**Initial acceptance:** Evaluation respects the declared order and rejects cycles.

**Scope:** `base` · **Level:** `advanced` · **Status:** `not_started`.

**Specification source:** `proposal`.

## CTL-008 — Portable controllers

Include dependencies and bindings when saving a rig template.

**Initial acceptance:** Importing the template into another scene preserves working controls.

**Scope:** `base` · **Level:** `advanced` · **Status:** `not_started`.

**Specification source:** `proposal`.
