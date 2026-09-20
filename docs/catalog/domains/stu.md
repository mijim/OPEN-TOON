# STU — Studio production and collaboration

[Back to catalog](../README.md)

> Generated from `features.json` and `domains.json`; do not edit manually.

**Workflow:** Assign scene → acquire rights → edit → version → deliver.

**Module:** `studio`.

**Entities:** Production, Job, SceneCheckout, AssetLock, RenderQueue.

**Relationships:** PRJ, LIB, OUT.

**Main risk:** Multi-user and network conflicts; this is not CRDT co-editing.

## Shared contract

Mutations must respect transactions, undo/redo and persistence. Visual aids are not exported. Errors, cancellation and unsupported data must preserve the last valid state. These OPEN-TOON conditions will be specified per operation during implementation.

## STU-001 — Productions and jobs

Organize scenes within production structures and assignments.

**Initial acceptance:** A scene has an identity independent of its disk path.

**Scope:** `studio_extension` · **Level:** `advanced` · **Status:** `not_started`.

**Evidence:** `proposal`.

## STU-002 — Shared scene access

Open and save scenes on shared storage through a defined service.

**Initial acceptance:** A user without permission is rejected without partial changes.

**Scope:** `studio_extension` · **Level:** `advanced` · **Status:** `not_started`.

**Evidence:** `proposal`.

## STU-003 — Scene locks

Coordinate write rights and lock release.

**Initial acceptance:** Two clients do not silently overwrite the same revision.

**Scope:** `studio_extension` · **Level:** `advanced` · **Status:** `not_started`.

**Evidence:** `proposal`.

## STU-004 — Drawing locks

Allow painting on authorized drawings while other roles work on the scene.

**Initial acceptance:** A drawing lock does not prevent editing a different authorized resource.

**Scope:** `studio_extension` · **Level:** `advanced` · **Status:** `not_started`.

**Evidence:** `proposal`.

## STU-005 — Delivery versions and merging

Manage revisions and detect divergence when reintegrating a scene.

**Initial acceptance:** An older delivery does not replace a newer one without conflict resolution.

**Scope:** `studio_extension` · **Level:** `advanced` · **Status:** `not_started`.

**Evidence:** `proposal`.

## STU-006 — Remote checkout

Package scenes for offline work and reintegrate changes.

**Initial acceptance:** A checkout retains a base revision for conflict detection.

**Scope:** `studio_extension` · **Level:** `advanced` · **Status:** `not_started`.

**Evidence:** `proposal`.

## STU-007 — Render and vectorization queues

Distribute jobs with status, retry and monitoring.

**Initial acceptance:** A disappearing worker leaves a recoverable job.

**Scope:** `studio_extension` · **Level:** `advanced` · **Status:** `not_started`.

**Evidence:** `proposal`.

## STU-008 — Paint and Scan roles

Provide task-limited workspaces for production painting and scanning.

**Initial acceptance:** The painting workspace does not expose commands that change protected timing.

**Scope:** `studio_extension` · **Level:** `advanced` · **Status:** `not_started`.

**Evidence:** `proposal`.
