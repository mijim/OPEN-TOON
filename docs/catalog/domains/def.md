# DEF — Deformers and meshes

[Back to catalog](../README.md)

> Generated from `features.json` and `domains.json`; do not edit manually.

**Workflow:** Prepare rest pose → define influences → animate → evaluate → bake.

**Module:** `deformation`.

**Entities:** DeformationChain, RestPose, Influence, Mesh, WeightMap.

**Relationships:** RIG.

**Main risk:** Deformation quality, self-intersections and complex textures.

## Shared contract

Mutations must respect transactions, undo/redo and persistence. Visual aids are not exported. Errors, cancellation and unsupported data must preserve the last valid state. These OPEN-TOON conditions will be specified per operation during implementation.

## DEF-001 — Bone deformation

Articulate a drawing through a chain of joints and influences.

**Initial acceptance:** A limb bends while maintaining connections between segments.

**Scope:** `base` · **Level:** `pro` · **Status:** `not_started`.

**Specification source:** `proposal`.

## DEF-002 — Game bones

Represent runtime-oriented skeletons with documented export limits.

**Initial acceptance:** Export preserves hierarchy and rest transforms.

**Scope:** `base` · **Level:** `advanced` · **Status:** `not_started`.

**Specification source:** `proposal`.

## DEF-003 — Curve deformation

Control shape through curve segments and tangents.

**Initial acceptance:** Moving a tangent produces a continuous transition.

**Scope:** `base` · **Level:** `pro` · **Status:** `not_started`.

**Specification source:** `proposal`.

## DEF-004 — Envelope deformation

Deform a silhouette through a closed envelope.

**Initial acceptance:** Closing the envelope introduces no discontinuity at its seam.

**Scope:** `base` · **Level:** `pro` · **Status:** `not_started`.

**Specification source:** `proposal`.

## DEF-005 — Free-form deformation

Deform interior regions with a mesh and internal controls.

**Initial acceptance:** A checker texture follows the interior deformation.

**Scope:** `base` · **Level:** `advanced` · **Status:** `not_started`.

**Specification source:** `proposal`.

## DEF-006 — Shape-aware deformation

Adapt weights to the shape using point, bone and cage controls.

**Initial acceptance:** Moving a localized control reduces influence outside its region.

**Scope:** `base` · **Level:** `advanced` · **Status:** `not_started`.

**Specification source:** `proposal`.

## DEF-007 — Weighted deformation

Combine curve, point and peg influences in a deformation field.

**Initial acceptance:** Overlapping sources blend without an abrupt boundary jump.

**Scope:** `base` · **Level:** `advanced` · **Status:** `not_started`.

**Specification source:** `proposal`.

## DEF-008 — Rest-pose editing

Separate bind/rest pose from animated state and update it explicitly.

**Initial acceptance:** Resetting returns to the rest pose without deleting keys.

**Scope:** `base` · **Level:** `pro` · **Status:** `not_started`.

**Specification source:** `proposal`.

## DEF-009 — Influence regions

Configure elliptical or shape-based regions and falloff radii.

**Initial acceptance:** Reducing a radius limits the deformed area without moving the control.

**Scope:** `base` · **Level:** `advanced` · **Status:** `not_started`.

**Specification source:** `proposal`.

## DEF-010 — Multiple-pose rigs

Assign deformation chains to compatible substitutions.

**Initial acceptance:** Changing drawings activates the correct chain without orphaned references.

**Scope:** `base` · **Level:** `advanced` · **Status:** `not_started`.

**Specification source:** `proposal`.

## DEF-011 — Kinematic output

Attach other elements to the evaluated result of a deformer.

**Initial acceptance:** An accessory follows the end of a deformed arm.

**Scope:** `base` · **Level:** `advanced` · **Status:** `not_started`.

**Specification source:** `proposal`.

## DEF-012 — Point kinematic output

Sample positions on a deformed curve to control other objects.

**Initial acceptance:** The attached object follows the chosen point throughout the animation.

**Scope:** `base` · **Level:** `advanced` · **Status:** `not_started`.

**Specification source:** `proposal`.

## DEF-013 — Convert deformation to drawings

Bake evaluated deformation into editable drawings.

**Initial acceptance:** The baked drawing reproduces the pose independently of the original rig.

**Scope:** `base` · **Level:** `pro` · **Status:** `not_started`.

**Specification source:** `proposal`.

## DEF-014 — Envelope generation

Derive initial controls from drawing geometry with subsequent cleanup.

**Initial acceptance:** The proposed envelope can be edited before confirmation.

**Scope:** `base` · **Level:** `advanced` · **Status:** `not_started`.

**Specification source:** `proposal`.

## DEF-015 — Deformation visibility and quality

Toggle controls and preview quality without changing the final result.

**Initial acceptance:** Hiding controls does not disable the deformer.

**Scope:** `base` · **Level:** `pro` · **Status:** `not_started`.

**Specification source:** `proposal`.
