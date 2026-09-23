# LYR — Layers, drawings and art sublayers

[Back to catalog](../README.md)

> Generated from `features.json` and `domains.json`; do not edit manually.

**Workflow:** Create layer → create drawing → expose → reuse or duplicate.

**Module:** `document`.

**Entities:** Layer, Drawing, DrawingId, ArtLayer, ExposureTrack.

**Relationships:** PRJ.

**Main risk:** Confusing exposures, shared drawings and independent copies.

## Shared contract

Mutations must respect transactions, undo/redo and persistence. Visual aids are not exported. Errors, cancellation and unsupported data must preserve the last valid state. These OPEN-TOON conditions will be specified per operation during implementation.

## LYR-001 — Typed layers

Create drawing, sound and transform layers with type-specific attributes.

**Initial acceptance:** The inspector prevents assigning audio samples to a vector layer.

**Scope:** `base` · **Level:** `core` · **Status:** `partial`.

**Specification source:** `proposal`.

**Implementation evidence:** [docs/implementation/status.json](../../../docs/implementation/status.json) — typed Drawing, Character, Peg, Part and Camera layers; sound layer remains open.

**Remaining scope:** Typed visual and transform layers work. Sound layers and their type-specific inspector validation remain open.

## LYR-002 — Reorder and rename layers

Change ordering and labels while retaining stable identifiers.

**Initial acceptance:** Renaming a layer does not break linked nodes or curves.

**Scope:** `base` · **Level:** `core` · **Status:** `partial`.

**Specification source:** `proposal`.

**Implementation evidence:** [docs/implementation/status.json](../../../docs/implementation/status.json) — experimental subset only, catalog acceptance not closed.

**Remaining scope:** The proposed behavior and acceptance test are OPEN-TOON requirements, not a claim of implementation.

## LYR-003 — Visibility, lock and solo

Separate editing visibility, locking and temporary isolation.

**Initial acceptance:** Leaving solo mode restores the original state of other layers.

**Scope:** `base` · **Level:** `core` · **Status:** `partial`.

**Specification source:** `proposal`.

**Implementation evidence:** [docs/implementation/status.json](../../../docs/implementation/status.json) — experimental subset only, catalog acceptance not closed.

**Remaining scope:** The proposed behavior and acceptance test are OPEN-TOON requirements, not a claim of implementation.

## LYR-004 — Layer groups

Group and ungroup layers while preserving relationships that affect the result.

**Initial acceptance:** Ungrouping does not change the drawings' world positions.

**Scope:** `base` · **Level:** `core` · **Status:** `not_started`.

**Specification source:** `proposal`.

## LYR-005 — Duplicate or clone

Distinguish a deep copy from an instance sharing drawings or functions.

**Initial acceptance:** Editing shared clone content updates its instances; editing a deep copy does not.

**Scope:** `base` · **Level:** `core` · **Status:** `partial`.

**Specification source:** `proposal`.

**Implementation evidence:** [docs/implementation/status.json](../../../docs/implementation/status.json) — experimental subset only, catalog acceptance not closed.

**Remaining scope:** The proposed behavior and acceptance test are OPEN-TOON requirements, not a claim of implementation.

## LYR-006 — Exposure synchronization

Allow linked tracks to share a drawing sequence.

**Initial acceptance:** Changing a synchronized exposure updates the associated tracks.

**Scope:** `base` · **Level:** `pro` · **Status:** `not_started`.

**Specification source:** `proposal`.

## LYR-007 — Element management

Identify used and unexposed drawings; renumber without losing references.

**Initial acceptance:** Clearing a cell does not delete its now-unexposed drawing.

**Scope:** `base` · **Level:** `pro` · **Status:** `partial`.

**Specification source:** `proposal`.

**Implementation evidence:** [docs/implementation/status.json](../../../docs/implementation/status.json) — clearing an exposure retains its independently stored drawing.

**Remaining scope:** Exposure clearing preserves source drawings. Used/unexposed drawing management and safe renumbering remain open.

## LYR-008 — Line, Color, Overlay and Underlay Art

Store four ordered art sublayers per drawing.

**Initial acceptance:** Painting Color Art leaves the Line Art strokes intact.

**Scope:** `base` · **Level:** `pro` · **Status:** `partial`.

**Specification source:** `proposal`.

**Implementation evidence:** [docs/implementation/status.json](../../../docs/implementation/status.json) — experimental subset only, catalog acceptance not closed.

**Remaining scope:** The proposed behavior and acceptance test are OPEN-TOON requirements, not a claim of implementation.

## LYR-009 — Art layer manager

Move, swap, duplicate and consolidate art sublayers across drawing ranges.

**Initial acceptance:** Swapping art layers across ten drawings is undone as one operation.

**Scope:** `base` · **Level:** `pro` · **Status:** `not_started`.

**Specification source:** `proposal`.

## LYR-010 — Sheet annotations

Add text or drawing annotations independently of the final render.

**Initial acceptance:** Annotations appear in the sheet but not in the exported sequence.

**Scope:** `base` · **Level:** `pro` · **Status:** `not_started`.

**Specification source:** `proposal`.

## LYR-011 — Multi-layer property editing

Apply shared attributes to a compatible selection of layers.

**Initial acceptance:** The operation reports incompatible layers and preserves the remaining data.

**Scope:** `base` · **Level:** `pro` · **Status:** `not_started`.

**Specification source:** `proposal`.
