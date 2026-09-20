# PAR — Particles

[Back to catalog](../README.md)

> Generated from `features.json` and `domains.json`; do not edit manually.

**Workflow:** Emit → apply forces → display → bake.

**Module:** `particles`.

**Entities:** ParticleSystem, Emitter, ParticleState, Force, SimulationCache.

**Relationships:** NOD, ANI.

**Main risk:** Simulation must be reproducible and support scrubbing.

## Shared contract

Mutations must respect transactions, undo/redo and persistence. Visual aids are not exported. Errors, cancellation and unsupported data must preserve the last valid state. These OPEN-TOON conditions will be specified per operation during implementation.

## PAR-001 — Sprite emitters

Emit images with controlled rate, lifetime and seed.

**Initial acceptance:** The same seed produces the same sequence.

**Scope:** `base` · **Level:** `advanced` · **Status:** `not_started`.

**Evidence:** `proposal`.

## PAR-002 — Emission regions

Define emission within planar or spatial regions.

**Initial acceptance:** Particles spawn only within the chosen region.

**Scope:** `base` · **Level:** `advanced` · **Status:** `not_started`.

**Evidence:** `proposal`.

## PAR-003 — Velocity and variation

Assign velocity, size, orientation and dispersion through parameters.

**Initial acceptance:** With zero variation, all particles share the initial value.

**Scope:** `base` · **Level:** `advanced` · **Status:** `not_started`.

**Evidence:** `proposal`.

## PAR-004 — Forces and motion

Apply gravity, wind, friction, vortices, attraction and repulsion.

**Initial acceptance:** With all forces at zero, initial velocity is preserved.

**Scope:** `base` · **Level:** `advanced` · **Status:** `not_started`.

**Evidence:** `proposal`.

## PAR-005 — Collisions and lifetime

Handle bounce, death and sink regions.

**Initial acceptance:** An expired particle no longer contributes to the render.

**Scope:** `base` · **Level:** `advanced` · **Status:** `not_started`.

**Evidence:** `proposal`.

## PAR-006 — Baking and compositing

Cache simulations and combine systems with other layers.

**Initial acceptance:** Jumping to a baked frame reproduces the sequential result.

**Scope:** `base` · **Level:** `advanced` · **Status:** `not_started`.

**Evidence:** `proposal`.

## PAR-007 — Particle templates

Save reusable systems with their assets and parameters.

**Initial acceptance:** Importing a template preserves seed and appearance.

**Scope:** `base` · **Level:** `advanced` · **Status:** `not_started`.

**Evidence:** `proposal`.
