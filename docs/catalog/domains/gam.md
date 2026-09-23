# GAM — Game output

[Back to catalog](../README.md)

> Generated from `features.json` and `domains.json`; do not edit manually.

**Workflow:** Prepare compatible rig → bake → export → validate runtime.

**Module:** `game-export`.

**Entities:** SpriteAtlas, RuntimeSkeleton, Anchor, CollisionShape.

**Relationships:** DEF, OUT, LIB.

**Main risk:** Not every node or deformer has a representation in game engines.

## Shared contract

Mutations must respect transactions, undo/redo and persistence. Visual aids are not exported. Errors, cancellation and unsupported data must preserve the last valid state. These OPEN-TOON conditions will be specified per operation during implementation.

## GAM-001 — Sprite sheets

Pack frames with pivot, size and timing metadata.

**Initial acceptance:** Reconstructing animation from the atlas preserves registration and timing.

**Scope:** `base` · **Level:** `pro` · **Status:** `not_started`.

**Specification source:** `proposal`.

## GAM-002 — Runtime skeleton and animation

Export a rig profile compatible with a target runtime.

**Initial acceptance:** An exported clip matches the defined reference poses.

**Scope:** `base` · **Level:** `advanced` · **Status:** `not_started`.

**Specification source:** `proposal`.

## GAM-003 — Effect baking

Convert unsupported parts into images or precomposed groups.

**Initial acceptance:** The report identifies elements that cease to be editable at runtime.

**Scope:** `base` · **Level:** `advanced` · **Status:** `not_started`.

**Specification source:** `proposal`.

## GAM-004 — Accessory anchors

Export attachment points and animated transforms.

**Initial acceptance:** An accessory stays attached to the hand in the runtime example.

**Scope:** `base` · **Level:** `advanced` · **Status:** `not_started`.

**Specification source:** `proposal`.

## GAM-005 — Collisions and metadata

Attach collision regions and game data to exports.

**Initial acceptance:** Collisions respect the asset's scale and origin.

**Scope:** `base` · **Level:** `advanced` · **Status:** `not_started`.

**Specification source:** `proposal`.

## GAM-006 — Runtime palette variants

Represent compatible variants or generate an atlas per variant.

**Initial acceptance:** Selecting a variant does not change animation timing.

**Scope:** `base` · **Level:** `advanced` · **Status:** `not_started`.

**Specification source:** `proposal`.

## GAM-007 — Legacy HTML output

Record EaselJS export as a historical compatibility option.

**Initial acceptance:** The specification distinguishes this format from the native OPEN-TOON format.

**Scope:** `legacy_candidate` · **Level:** `legacy` · **Status:** `not_started`.

**Specification source:** `proposal`.
