# RAS — Bitmap drawing, textures and brushes

[Back to catalog](../README.md)

> Generated from `features.json` and `domains.json`; do not edit manually.

**Workflow:** Draw bitmap → edit → manage resolution → exchange brushes.

**Module:** `drawing-raster`.

**Entities:** RasterTile, BrushPreset, TextureAsset, RasterSelection.

**Relationships:** LYR, UI.

**Main risk:** Memory usage and consistency between texture and transform.

## Shared contract

Mutations must respect transactions, undo/redo and persistence. Visual aids are not exported. Errors, cancellation and unsupported data must preserve the last valid state. These OPEN-TOON conditions will be specified per operation during implementation.

## RAS-001 — Bitmap layers

Paint pixel images with a defined resolution and transparency.

**Initial acceptance:** An empty layer exports with zero alpha.

**Scope:** `base` · **Level:** `core` · **Status:** `partial`.

**Evidence:** `proposal`.

## RAS-002 — Textured bitmap brush

Support brush tip, spacing, opacity and dynamics for raster strokes.

**Initial acceptance:** A long stroke maintains stable density as drawing speed changes.

**Scope:** `base` · **Level:** `core` · **Status:** `partial`.

**Evidence:** `proposal`.

## RAS-003 — Bitmap eraser

Remove or reduce alpha with configurable edge softness and opacity.

**Initial acceptance:** Erasing does not introduce opaque black pixels at the edge.

**Scope:** `base` · **Level:** `core` · **Status:** `partial`.

**Evidence:** `proposal`.

## RAS-004 — Texture resolution

Change image resolution and quality while explaining resampling loss.

**Initial acceptance:** Downsampling preserves the agreed physical dimensions.

**Scope:** `base` · **Level:** `pro` · **Status:** `not_started`.

**Evidence:** `proposal`.

## RAS-005 — Raster selection

Transform selected pixels while preserving the mask and alpha channel.

**Initial acceptance:** Rotating the selection does not fill the surrounding area with a solid color.

**Scope:** `base` · **Level:** `core` · **Status:** `partial`.

**Evidence:** `proposal`.

## RAS-006 — ABR brushes

Import a supported subset of Photoshop brushes and report ignored parameters.

**Initial acceptance:** An unsupported ABR file is rejected with diagnostics without creating a broken preset.

**Scope:** `base` · **Level:** `pro` · **Status:** `not_started`.

**Evidence:** `proposal`.

## RAS-007 — Vector brush with bitmap texture

Keep vector geometry separate from resolution-dependent raster texture.

**Initial acceptance:** The editor warns when magnification exceeds the texture resolution.

**Scope:** `base` · **Level:** `pro` · **Status:** `not_started`.

**Evidence:** `proposal`.
