# FX — Effects, advanced compositing and shading

[Back to catalog](../README.md)

> Generated from `features.json` and `domains.json`; do not edit manually.

**Workflow:** Add effect → define bounds → animate parameters → verify output.

**Module:** `effects`.

**Entities:** EffectDefinition, AnimatedParameter, Matte, SurfaceMap.

**Relationships:** NOD, COL.

**Main risk:** Color precision, alpha, tile edges and cumulative cost.

## Shared contract

Mutations must respect transactions, undo/redo and persistence. Visual aids are not exported. Errors, cancellation and unsupported data must preserve the last valid state. These OPEN-TOON conditions will be specified per operation during implementation.

## FX-001 — Blend modes

Combine images using documented blending and alpha equations.

**Initial acceptance:** A patch test suite matches the selected equations.

**Scope:** `base` · **Level:** `pro` · **Status:** `not_started`.

**Specification source:** `proposal`.

## FX-002 — Animated transparency

Control alpha independently of RGB.

**Initial acceptance:** Reducing opacity to zero produces zero alpha without a halo when recomposited.

**Scope:** `base` · **Level:** `core` · **Status:** `not_started`.

**Specification source:** `proposal`.

## FX-003 — Gaussian and box blur

Filter by radius with explicit edge extension.

**Initial acceptance:** Tiled blur leaves no seams.

**Scope:** `base` · **Level:** `pro` · **Status:** `not_started`.

**Specification source:** `proposal`.

## FX-004 — Directional, radial and zoom blur

Apply blur with animatable center, direction and length.

**Initial acceptance:** Moving the center outside the frame preserves defined behavior.

**Scope:** `base` · **Level:** `pro` · **Status:** `not_started`.

**Specification source:** `proposal`.

## FX-005 — Bokeh and variable blur

Control blur using shapes and spatial maps.

**Initial acceptance:** Areas with zero radius preserve the input.

**Scope:** `base` · **Level:** `advanced` · **Status:** `not_started`.

**Specification source:** `proposal`.

## FX-006 — Glow and bloom

Generate light halos while preserving HDR when enabled.

**Initial acceptance:** An HDR source is not clipped before filtering in float mode.

**Scope:** `base` · **Level:** `pro` · **Status:** `not_started`.

**Specification source:** `proposal`.

## FX-007 — Shadows, tones and highlights

Create 2D shading from silhouettes and mattes.

**Initial acceptance:** A shadow can move independently of the source drawing.

**Scope:** `base` · **Level:** `pro` · **Status:** `not_started`.

**Specification source:** `proposal`.

## FX-008 — Color curves and levels

Adjust channels through animatable curves and ranges.

**Initial acceptance:** An identity curve returns the input within tolerance.

**Scope:** `base` · **Level:** `pro` · **Status:** `not_started`.

**Specification source:** `proposal`.

## FX-009 — Color override

Replace swatches and textures by identity on selected branches.

**Initial acceptance:** The same drawing can render with two variants simultaneously.

**Scope:** `base` · **Level:** `pro` · **Status:** `not_started`.

**Specification source:** `proposal`.

## FX-010 — Palette fades

Interpolate palette or swatch colors over time.

**Initial acceptance:** The fade does not change swatch identity.

**Scope:** `base` · **Level:** `pro` · **Status:** `not_started`.

**Specification source:** `proposal`.

## FX-011 — Texture transforms

Animate fill coordinates without moving geometry.

**Initial acceptance:** A texture moves inside a stationary contour.

**Scope:** `base` · **Level:** `pro` · **Status:** `not_started`.

**Specification source:** `proposal`.

## FX-012 — Sequence textures

Replace a texture with frames from an external sequence.

**Initial acceptance:** Texture sampling respects the defined frame rate and range.

**Scope:** `base` · **Level:** `advanced` · **Status:** `not_started`.

**Specification source:** `proposal`.

## FX-013 — Animated matte

Edit animatable contours with inner and outer feathering.

**Initial acceptance:** The mask remains continuous when interpolating compatible controls.

**Scope:** `base` · **Level:** `advanced` · **Status:** `not_started`.

**Specification source:** `proposal`.

## FX-014 — Channels and keying

Select, swap and extract mattes from image channels.

**Initial acceptance:** The selected channel matches the original test values.

**Scope:** `base` · **Level:** `pro` · **Status:** `not_started`.

**Specification source:** `proposal`.

## FX-015 — Generators

Create solid color, gradients, grids and noise as image sources.

**Initial acceptance:** A generator with a fixed seed is reproducible.

**Scope:** `base` · **Level:** `pro` · **Status:** `not_started`.

**Specification source:** `proposal`.

## FX-016 — Distortion and turbulence

Deform sampling coordinates using maps and parameters.

**Initial acceptance:** Identity distortion produces the original input.

**Scope:** `base` · **Level:** `advanced` · **Status:** `not_started`.

**Specification source:** `proposal`.

## FX-017 — Sharpening and antiflicker

Provide detail and visual-stability filters within bounded ranges.

**Initial acceptance:** A disabled filter does not modify pixels.

**Scope:** `base` · **Level:** `pro` · **Status:** `not_started`.

**Specification source:** `proposal`.

## FX-018 — Light shading and normals

Light artwork using normals or auxiliary volumes.

**Initial acceptance:** Moving a light changes shading without modifying the source drawing.

**Scope:** `base` · **Level:** `advanced` · **Status:** `not_started`.

**Specification source:** `proposal`.

## FX-019 — Surface shading and cast shadows

Define surfaces and elevation for shadows and occlusion.

**Initial acceptance:** Surfaces at different heights produce the expected shadow relationship.

**Scope:** `base` · **Level:** `advanced` · **Status:** `not_started`.

**Specification source:** `proposal`.

## FX-020 — OpenFX

Host a declared subset of the standard with compatible plugins.

**Initial acceptance:** An incompatible plugin is rejected and remains identified in the project.

**Scope:** `base` · **Level:** `advanced` · **Status:** `not_started`.

**Specification source:** `proposal`.
