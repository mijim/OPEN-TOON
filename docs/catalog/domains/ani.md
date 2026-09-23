# ANI — Transforms, curves and keyframe animation

[Back to catalog](../README.md)

> Generated from `features.json` and `domains.json`; do not edit manually.

**Workflow:** Place keys → interpolate → adjust curves → reuse motion.

**Module:** `animation-curves`.

**Entities:** AnimatableProperty, Keyframe, Curve, Transform, Expression.

**Relationships:** TIM, LYR.

**Main risk:** Matrix order, pivots and curve evaluation.

## Shared contract

Mutations must respect transactions, undo/redo and persistence. Visual aids are not exported. Errors, cancellation and unsupported data must preserve the last valid state. These OPEN-TOON conditions will be specified per operation during implementation.

## ANI-001 — Layer transforms

Animate translation, rotation, scale, skew and opacity with consistent units.

**Initial acceptance:** Negative scale does not produce undefined interpolation values.

**Scope:** `base` · **Level:** `core` · **Status:** `partial`.

**Specification source:** `proposal`.

**Implementation evidence:** [docs/implementation/status.json](../../../docs/implementation/status.json) — docs/architecture/adr/016-visual-animation-and-bezier.md; bounded visual-editing subset only, catalog acceptance not closed.

**Remaining scope:** The proposed behavior and acceptance test are OPEN-TOON requirements, not a claim of implementation.

## ANI-002 — Setup and animate modes

Separate rest-state changes from keyframe insertion or editing.

**Initial acceptance:** Moving an object in setup mode does not add keyframes.

**Scope:** `base` · **Level:** `core` · **Status:** `partial`.

**Specification source:** `proposal`.

**Implementation evidence:** [docs/implementation/status.json](../../../docs/implementation/status.json) — docs/architecture/adr/016-visual-animation-and-bezier.md; bounded visual-editing subset only, catalog acceptance not closed.

**Remaining scope:** The proposed behavior and acceptance test are OPEN-TOON requirements, not a claim of implementation.

## ANI-003 — Pegs

Use transform nodes independent of graphic content.

**Initial acceptance:** Moving a peg transforms all its children without changing their drawings.

**Scope:** `base` · **Level:** `core` · **Status:** `partial`.

**Specification source:** `proposal`.

**Implementation evidence:** [docs/implementation/HM03-RIG-FOUNDATION.md](../../../docs/implementation/HM03-RIG-FOUNDATION.md) — independent parent Peg transforms move child artwork.

**Remaining scope:** Rigid pegs work. The full animation/rigging interactions and production acceptance remain open.

## ANI-004 — Interpolated and held keys

Support continuous segments and controlled pose changes.

**Initial acceptance:** A stepped segment holds its pose until the next keyframe.

**Scope:** `base` · **Level:** `core` · **Status:** `partial`.

**Specification source:** `proposal`.

**Implementation evidence:** [docs/architecture/adr/021-vector-authoring-and-key-patterns.md](../../../docs/architecture/adr/021-vector-authoring-and-key-patterns.md) — Batch full-pose Linear/Step/Smooth interpolation on one layer; independent channel keys pending..

**Remaining scope:** The proposed behavior and acceptance test are OPEN-TOON requirements, not a claim of implementation.

## ANI-005 — Autokey and key editing

Create keys per property or property set without accidental keys.

**Initial acceptance:** With autokey disabled, local edits are distinguished from animated edits.

**Scope:** `base` · **Level:** `core` · **Status:** `partial`.

**Specification source:** `proposal`.

**Implementation evidence:** [docs/implementation/status.json](../../../docs/implementation/status.json) — docs/architecture/adr/016-visual-animation-and-bezier.md; bounded visual-editing subset only, catalog acceptance not closed.

**Remaining scope:** The proposed behavior and acceptance test are OPEN-TOON requirements, not a claim of implementation.

## ANI-006 — Function editor

Edit curves with visible tangents, ranges, values and units.

**Initial acceptance:** An edited tangent produces the same value in preview and export.

**Scope:** `base` · **Level:** `core` · **Status:** `partial`.

**Specification source:** `proposal`.

**Implementation evidence:** [docs/implementation/status.json](../../../docs/implementation/status.json) — docs/architecture/adr/016-visual-animation-and-bezier.md; docs/architecture/adr/017-combined-motion-and-point-editing.md; bounded visual-editing subset only, catalog acceptance not closed.

**Remaining scope:** The proposed behavior and acceptance test are OPEN-TOON requirements, not a claim of implementation.

## ANI-007 — Easing and velocity

Control acceleration and deceleration through curves and presets.

**Initial acceptance:** Easing does not move keyframe endpoints.

**Scope:** `base` · **Level:** `core` · **Status:** `partial`.

**Specification source:** `proposal`.

**Implementation evidence:** [docs/architecture/adr/021-vector-authoring-and-key-patterns.md](../../../docs/architecture/adr/021-vector-authoring-and-key-patterns.md) — Batch outgoing easing/overshoot presets across full-pose channels; independent spatial velocity pending..

**Remaining scope:** The proposed behavior and acceptance test are OPEN-TOON requirements, not a claim of implementation.

## ANI-008 — Motion paths and spatial velocity

Separate the path shape from temporal progress along it.

**Initial acceptance:** Changing speed preserves the path geometry.

**Scope:** `base` · **Level:** `pro` · **Status:** `partial`.

**Specification source:** `proposal`.

**Implementation evidence:** [docs/implementation/status.json](../../../docs/implementation/status.json) — docs/architecture/adr/019-canvas-motion-path-editing.md; canvas pose-position edits on an evaluated trajectory only; independent spatial geometry and velocity remain unimplemented.

**Remaining scope:** The proposed behavior and acceptance test are OPEN-TOON requirements, not a claim of implementation.

## ANI-009 — Multi-key editing

Move, scale and copy key blocks between compatible tracks.

**Initial acceptance:** Scaling a block preserves order or explains timing collisions.

**Scope:** `base` · **Level:** `pro` · **Status:** `partial`.

**Specification source:** `proposal`.

**Implementation evidence:** [docs/architecture/adr/021-vector-authoring-and-key-patterns.md](../../../docs/architecture/adr/021-vector-authoring-and-key-patterns.md) — Sparse key-block repeat with preserved poses/easing, duration extension and collision rejection; channel masks and cross-layer batching pending..

**Remaining scope:** The proposed behavior and acceptance test are OPEN-TOON requirements, not a claim of implementation.

## ANI-010 — Copy motion

Transfer animation between objects with explicit pivot and unit rules.

**Initial acceptance:** Copying motion does not duplicate the underlying drawing.

**Scope:** `base` · **Level:** `pro` · **Status:** `partial`.

**Specification source:** `proposal`.

**Implementation evidence:** [docs/implementation/status.json](../../../docs/implementation/status.json) — docs/architecture/adr/018-pose-key-block-editing.md; single-layer full-pose block operations in local units only; catalog acceptance not closed.

**Remaining scope:** The proposed behavior and acceptance test are OPEN-TOON requirements, not a claim of implementation.

## ANI-011 — Expressions

Evaluate attribute relationships with cycle detection and visible errors.

**Initial acceptance:** A circular dependency does not block the application.

**Scope:** `base` · **Level:** `advanced` · **Status:** `not_started`.

**Specification source:** `proposal`.

## ANI-012 — Manual motion capture

Record a gesture path as editable time-based data.

**Initial acceptance:** The captured path can be simplified and the operation undone.

**Scope:** `base` · **Level:** `pro` · **Status:** `not_started`.

**Specification source:** `proposal`.

## ANI-013 — Animatable numeric editor

Edit values and navigate keys directly from the inspector.

**Initial acceptance:** The inspector indicates whether a value is keyed or interpolated.

**Scope:** `base` · **Level:** `pro` · **Status:** `partial`.

**Specification source:** `proposal`.

**Implementation evidence:** [docs/implementation/status.json](../../../docs/implementation/status.json) — experimental subset only, catalog acceptance not closed.

**Remaining scope:** The proposed behavior and acceptance test are OPEN-TOON requirements, not a claim of implementation.
