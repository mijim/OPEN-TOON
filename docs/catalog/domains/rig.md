# RIG — Rigging, hierarchies and inverse kinematics

[Back to catalog](../README.md)

> Generated from `features.json` and `domains.json`; do not edit manually.

**Workflow:** Break down → set pivots → build hierarchy → add controls → pose.

**Module:** `rigging`.

**Entities:** Rig, Joint, PegHierarchy, Constraint, DrawingSubstitution.

**Relationships:** ANI, VEC.

**Main risk:** Dependency cycles and pose jumps when changing hierarchies.

## Shared contract

Mutations must respect transactions, undo/redo and persistence. Visual aids are not exported. Errors, cancellation and unsupported data must preserve the last valid state. These OPEN-TOON conditions will be specified per operation during implementation.

## RIG-001 — Character breakdown

Separate model parts while preserving registration and palettes.

**Initial acceptance:** The pieces visually reconstruct the original character at rest.

**Scope:** `base` · **Level:** `pro` · **Status:** `not_started`.

**Evidence:** `proposal`.

## RIG-002 — Transform hierarchies

Build parent-child chains with separate drawings and pegs.

**Initial acceptance:** Reparenting with preserve-world enabled does not move the character.

**Scope:** `base` · **Level:** `core` · **Status:** `not_started`.

**Evidence:** `proposal`.

## RIG-003 — Permanent and temporary pivots

Edit the rotation center with an explicit scope distinction.

**Initial acceptance:** Moving the temporary pivot does not change the rig's saved pivot.

**Scope:** `base` · **Level:** `core` · **Status:** `not_started`.

**Evidence:** `proposal`.

## RIG-004 — Z order and depth adjustment

Control part overlap with fine depth adjustments.

**Initial acceptance:** An arm can move in front of and behind the torso with stable results.

**Scope:** `base` · **Level:** `pro` · **Status:** `not_started`.

**Evidence:** `proposal`.

## RIG-005 — Joints and Auto Patch

Resolve joints and hide lines between overlapping parts.

**Initial acceptance:** The test joint shows no seam when the arm bends.

**Scope:** `base` · **Level:** `pro` · **Status:** `not_started`.

**Evidence:** `proposal`.

## RIG-006 — Drawing substitutions

Swap mouths, hands or views of a part while retaining animation.

**Initial acceptance:** Changing a mouth does not alter the head transform.

**Scope:** `base` · **Level:** `core` · **Status:** `not_started`.

**Evidence:** `proposal`.

## RIG-007 — Multiple substitutions

Switch coordinated variants across several character elements.

**Initial acceptance:** Changing a view replaces the assigned parts in one transaction.

**Scope:** `base` · **Level:** `pro` · **Status:** `not_started`.

**Evidence:** `proposal`.

## RIG-008 — Hierarchy navigation

Select a parent, child or chain without manually finding every node.

**Initial acceptance:** Navigation respects the group and does not cross into another character.

**Scope:** `base` · **Level:** `pro` · **Status:** `not_started`.

**Evidence:** `proposal`.

## RIG-009 — Inverse kinematics

Solve joints from a target with configurable limits.

**Initial acceptance:** Dragging a hand preserves bone lengths when the selected mode requires rigidity.

**Scope:** `base` · **Level:** `advanced` · **Status:** `not_started`.

**Evidence:** `proposal`.

## RIG-010 — Nails and IK constraints

Pin points while manipulating other parts.

**Initial acceptance:** A pinned foot stays in position while the torso moves.

**Scope:** `base` · **Level:** `advanced` · **Status:** `not_started`.

**Evidence:** `proposal`.

## RIG-011 — Constraint keyframes

Animate constraint states and control transitions.

**Initial acceptance:** Enabling a pin on one frame does not change earlier frames.

**Scope:** `base` · **Level:** `advanced` · **Status:** `not_started`.

**Evidence:** `proposal`.

## RIG-012 — Copy poses

Save and transfer poses using stable part mappings.

**Initial acceptance:** Applying a pose does not overwrite properties outside its set.

**Scope:** `base` · **Level:** `pro` · **Status:** `not_started`.

**Evidence:** `proposal`.

## RIG-013 — Breakdown Assistant

Blend neighboring poses by percentage and selected attributes.

**Initial acceptance:** Zero and one hundred percent reproduce the reference poses.

**Scope:** `base` · **Level:** `pro` · **Status:** `not_started`.

**Evidence:** `proposal`.

## RIG-014 — Rig guides and conventions

Identify controls, guides and names without including them in the render.

**Initial acceptance:** A guide visible during setup is excluded from final output.

**Scope:** `base` · **Level:** `pro` · **Status:** `not_started`.

**Evidence:** `proposal`.
