# CAM — Camera, staging and 2.5D space

[Back to catalog](../README.md)

> Generated from `features.json` and `domains.json`; do not edit manually.

**Workflow:** Position planes → frame shot → animate camera → check composition.

**Module:** `camera`.

**Entities:** Camera, Projection, StagePlane, SafeArea.

**Relationships:** ANI.

**Main risk:** Projection and depth must agree across views.

## Shared contract

Mutations must respect transactions, undo/redo and persistence. Visual aids are not exported. Errors, cancellation and unsupported data must preserve the last valid state. These OPEN-TOON conditions will be specified per operation during implementation.

## CAM-001 — Cameras and active camera

Create cameras and explicitly select the output camera.

**Initial acceptance:** Changing the active camera changes preview and export consistently.

**Scope:** `base` · **Level:** `core` · **Status:** `not_started`.

**Evidence:** `proposal`.

## CAM-002 — Camera transforms

Animate position, rotation and framing through pegs and curves.

**Initial acceptance:** Camera motion does not change local drawing coordinates.

**Scope:** `base` · **Level:** `pro` · **Status:** `not_started`.

**Evidence:** `proposal`.

## CAM-003 — Multiplane

Distribute layers in depth to produce parallax.

**Initial acceptance:** A tracking shot creates different displacement according to depth.

**Scope:** `base` · **Level:** `pro` · **Status:** `not_started`.

**Evidence:** `proposal`.

## CAM-004 — Top, side and perspective views

Inspect spatial positions and camera relationships.

**Initial acceptance:** Selection identifies the same object across views.

**Scope:** `base` · **Level:** `pro` · **Status:** `not_started`.

**Evidence:** `proposal`.

## CAM-005 — Orthographic projection

Provide perspective-free output for 2D and game workflows.

**Initial acceptance:** Equal objects at different depths retain the same apparent size.

**Scope:** `base` · **Level:** `pro` · **Status:** `not_started`.

**Evidence:** `proposal`.

## CAM-006 — Framing guides

Provide alignment guides and reference zones that are not exported.

**Initial acceptance:** Guides are absent from the rendered image.

**Scope:** `base` · **Level:** `core` · **Status:** `not_started`.

**Evidence:** `proposal`.

## CAM-007 — Staging transforms and opacity

Position backgrounds and references with locking and temporary transparency.

**Initial acceptance:** Working opacity is distinguished from renderable opacity.

**Scope:** `base` · **Level:** `core` · **Status:** `not_started`.

**Evidence:** `proposal`.
