# MOR — Vector morphing

[Back to catalog](../README.md)

> Generated from `features.json` and `domains.json`; do not edit manually.

**Workflow:** Choose endpoints → establish correspondences → interpolate → correct.

**Module:** `morphing`.

**Entities:** MorphSequence, CorrespondenceHint, MorphKey.

**Relationships:** VEC, TIM.

**Main risk:** Incompatible topologies, holes and degenerate strokes.

## Shared contract

Mutations must respect transactions, undo/redo and persistence. Visual aids are not exported. Errors, cancellation and unsupported data must preserve the last valid state. These OPEN-TOON conditions will be specified per operation during implementation.

## MOR-001 — Drawing interpolation

Generate in-betweens between compatible vector drawings.

**Initial acceptance:** The first and last frames are identical to the endpoint drawings.

**Scope:** `base` · **Level:** `advanced` · **Status:** `not_started`.

**Evidence:** `proposal`.

## MOR-002 — Compatibility rules

Diagnose region and color incompatibilities before interpolation.

**Initial acceptance:** Bitmap input is rejected in vector mode.

**Scope:** `base` · **Level:** `advanced` · **Status:** `not_started`.

**Evidence:** `proposal`.

## MOR-003 — Correspondence hints

Let artists guide matching points, contours or lines.

**Initial acceptance:** Adding a correspondence changes the intended segment.

**Scope:** `base` · **Level:** `advanced` · **Status:** `not_started`.

**Evidence:** `proposal`.

## MOR-004 — Intermediate morph keys

Turn an intermediate result into a key drawing for additional detail.

**Initial acceptance:** The new key splits the morph and remains editable.

**Scope:** `base` · **Level:** `advanced` · **Status:** `not_started`.

**Evidence:** `proposal`.

## MOR-005 — Morph timing and easing

Separate temporal progress from geometric correspondence.

**Initial acceptance:** Adjusting easing preserves endpoint drawings and duration.

**Scope:** `base` · **Level:** `advanced` · **Status:** `not_started`.

**Evidence:** `proposal`.

## MOR-006 — Holes and transparency

Resolve interior regions and alpha under visible rules.

**Initial acceptance:** A ring retains its hole throughout a valid sequence.

**Scope:** `base` · **Level:** `advanced` · **Status:** `not_started`.

**Evidence:** `proposal`.
