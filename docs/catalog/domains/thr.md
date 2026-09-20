# THR — 3D integration

[Back to catalog](../README.md)

> Generated from `features.json` and `domains.json`; do not edit manually.

**Workflow:** Import model → adjust units → pose → composite with 2D.

**Module:** `integration-3d`.

**Entities:** ModelAsset, ModelSubnode, Skeleton, AnimationClip, ExternalRenderJob.

**Relationships:** CAM, NOD.

**Main risk:** Format compatibility, axes and external rendering cost.

## Shared contract

Mutations must respect transactions, undo/redo and persistence. Visual aids are not exported. Errors, cancellation and unsupported data must preserve the last valid state. These OPEN-TOON conditions will be specified per operation during implementation.

## THR-001 — Import 3D models

Read supported formats with materials and hierarchies under a documented profile.

**Initial acceptance:** A reference model retains proportions and orientation.

**Scope:** `base` · **Level:** `advanced` · **Status:** `not_started`.

**Evidence:** `proposal`.

## THR-002 — Units and scale

Interpret file units and convert them to scene space.

**Initial acceptance:** A one-meter object retains a ten-to-one relationship with a ten-centimeter object.

**Scope:** `base` · **Level:** `advanced` · **Status:** `not_started`.

**Evidence:** `proposal`.

## THR-003 — 3D animation clips

Select and retime animations included in an asset.

**Initial acceptance:** A cyclic repeated clip has no jump at its boundaries.

**Scope:** `base` · **Level:** `advanced` · **Status:** `not_started`.

**Evidence:** `proposal`.

## THR-004 — Subnodes

Select and animate internal model components.

**Initial acceptance:** Moving one part does not move an unlinked sibling.

**Scope:** `base` · **Level:** `advanced` · **Status:** `not_started`.

**Evidence:** `proposal`.

## THR-005 — Armatures and skinning

Animate model skeletons and deformation within supported limits.

**Initial acceptance:** The test rig retains its bind pose on import.

**Scope:** `base` · **Level:** `advanced` · **Status:** `not_started`.

**Evidence:** `proposal`.

## THR-006 — Subnode overrides

Bake imported transforms and clips into editable keys.

**Initial acceptance:** Baking reproduces sampled positions from the original clip.

**Scope:** `base` · **Level:** `advanced` · **Status:** `not_started`.

**Evidence:** `proposal`.

## THR-007 — 2D and 3D compositing

Combine drawn planes and models with consistent depth, transparency and cameras.

**Initial acceptance:** An object crosses planes in the expected order.

**Scope:** `base` · **Level:** `advanced` · **Status:** `not_started`.

**Evidence:** `proposal`.

## THR-008 — External rendering

Orchestrate external renderers with explicit resources, cancellation and errors.

**Initial acceptance:** A render process failure does not close the editor.

**Scope:** `base` · **Level:** `advanced` · **Status:** `not_started`.

**Evidence:** `proposal`.

## THR-009 — Depth normalization

Handle depth boundaries and flattening of 3D results.

**Initial acceptance:** Edge compositing produces no halos from invalid depth.

**Scope:** `base` · **Level:** `advanced` · **Status:** `not_started`.

**Evidence:** `proposal`.
