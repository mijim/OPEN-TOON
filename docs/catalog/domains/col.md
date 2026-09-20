# COL — Painting, palettes and color management

[Back to catalog](../README.md)

> Generated from `features.json` and `domains.json`; do not edit manually.

**Workflow:** Define palette → fill → correct → create variants → verify color.

**Module:** `colour`.

**Entities:** Palette, SwatchId, Gradient, TextureFill, ColourConfig.

**Relationships:** LYR, VEC, RAS.

**Main risk:** Confusing color identity with RGB values and losing production consistency.

## Shared contract

Mutations must respect transactions, undo/redo and persistence. Visual aids are not exported. Errors, cancellation and unsupported data must preserve the last valid state. These OPEN-TOON conditions will be specified per operation during implementation.

## COL-001 — Palettes and identified colors

Reference colors using stable identifiers separate from RGBA values.

**Initial acceptance:** Changing a swatch recolors all its references and no distinct swatch.

**Scope:** `base` · **Level:** `core` · **Status:** `partial`.

**Evidence:** `proposal`.

## COL-002 — Create and edit swatches

Manage solid swatches, names, opacity and duplicates.

**Initial acceptance:** Two swatches with equal RGB values can retain different identities.

**Scope:** `base` · **Level:** `core` · **Status:** `partial`.

**Evidence:** `proposal`.

## COL-003 — Region fill

Paint closed regions using a defined tolerance.

**Initial acceptance:** An adjacent region separated by a line is not painted.

**Scope:** `base` · **Level:** `core` · **Status:** `not_started`.

**Evidence:** `proposal`.

## COL-004 — Paint lines and areas

Recolor outlines and fills independently.

**Initial acceptance:** Painting only lines preserves interior colors.

**Scope:** `base` · **Level:** `core` · **Status:** `partial`.

**Evidence:** `proposal`.

## COL-005 — Gap closing

Resolve small discontinuities to support filling without joining arbitrary areas.

**Initial acceptance:** A gap larger than the tolerance remains open.

**Scope:** `base` · **Level:** `pro` · **Status:** `not_started`.

**Evidence:** `proposal`.

## COL-006 — Multi-drawing painting

Apply paint operations to selected drawings or a time range.

**Initial acceptance:** The summary identifies which unique drawings changed.

**Scope:** `base` · **Level:** `pro` · **Status:** `not_started`.

**Evidence:** `proposal`.

## COL-007 — Gradients and textures

Use gradient and texture fills with independent transforms.

**Initial acceptance:** Rotating a texture does not change its clipping shape.

**Scope:** `base` · **Level:** `pro` · **Status:** `not_started`.

**Evidence:** `proposal`.

## COL-008 — Color model

Compare and sample a project-associated color reference.

**Initial acceptance:** Changing the reference model does not automatically modify the drawing.

**Scope:** `base` · **Level:** `pro` · **Status:** `not_started`.

**Evidence:** `proposal`.

## COL-009 — Palette lists

Resolve scene, element and production palettes in an explicit order.

**Initial acceptance:** Identity conflicts are resolved reproducibly.

**Scope:** `base` · **Level:** `pro` · **Status:** `not_started`.

**Evidence:** `proposal`.

## COL-010 — Palette clones and variants

Share identities between color variations without manual repainting.

**Initial acceptance:** A night variant affects only the configured instance.

**Scope:** `base` · **Level:** `pro` · **Status:** `not_started`.

**Evidence:** `proposal`.

## COL-011 — Import and export palettes

Transfer palettes with texture resources and conflict rules.

**Initial acceptance:** Repeated import does not silently duplicate identities.

**Scope:** `base` · **Level:** `pro` · **Status:** `not_started`.

**Evidence:** `proposal`.

## COL-012 — Recover colors

Restore or reassign colors whose resources are unavailable.

**Initial acceptance:** Opening without a palette reports the problem without replacing original IDs.

**Scope:** `base` · **Level:** `core` · **Status:** `not_started`.

**Evidence:** `proposal`.

## COL-013 — Palette optimization

Detect redundant or unused swatches while respecting existing revisions.

**Initial acceptance:** A swatch used in a historical version is not deleted by default.

**Scope:** `base` · **Level:** `pro` · **Status:** `not_started`.

**Evidence:** `proposal`.

## COL-014 — Color spaces

Distinguish input interpretation, compositing, display and output.

**Initial acceptance:** A reference chart completes the pipeline with measured error.

**Scope:** `base` · **Level:** `pro` · **Status:** `not_started`.

**Evidence:** `proposal`.

## COL-015 — Studio color configurations

Support a shared, validated color configuration.

**Initial acceptance:** A missing configuration produces a warning rather than a silent conversion.

**Scope:** `base` · **Level:** `advanced` · **Status:** `not_started`.

**Evidence:** `proposal`.
