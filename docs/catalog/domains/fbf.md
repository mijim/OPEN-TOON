# FBF — Traditional and paperless animation

[Back to catalog](../README.md)

> Generated from `features.json` and `domains.json`; do not edit manually.

**Workflow:** Rough → tie-down → cleanup → paint → review.

**Module:** `animation-drawing`.

**Entities:** OnionSettings, TraceOffset, DrawingMark, DrawingDesk.

**Relationships:** TIM, VEC, COL.

**Main risk:** Visual aids must never accidentally become exported artwork.

## Shared contract

Mutations must respect transactions, undo/redo and persistence. Visual aids are not exported. Errors, cancellation and unsupported data must preserve the last valid state. These OPEN-TOON conditions will be specified per operation during implementation.

## FBF-001 — Frame-by-frame drawing

Create and navigate drawings with continuous tool and exposure state.

**Initial acceptance:** Drawing on a new frame does not modify the previous drawing without explicit intent.

**Scope:** `base` · **Level:** `core` · **Status:** `partial`.

**Evidence:** `proposal`.

## FBF-002 — Rough, tie-down and cleanup

Organize drawing stages through layers, marks and visibility.

**Initial acceptance:** Hiding rough layers leaves a complete cleanup without missing references.

**Scope:** `base` · **Level:** `core` · **Status:** `not_started`.

**Evidence:** `proposal`.

## FBF-003 — Previous and next onion skin

Show neighboring drawings with configurable range, opacity and visual distinction.

**Initial acceptance:** Onion skin is absent from the final render.

**Scope:** `base` · **Level:** `core` · **Status:** `partial`.

**Evidence:** `proposal`.

## FBF-004 — Advanced onion skin

Select specific drawings, distinguish frames from unique drawings and compare layers.

**Initial acceptance:** A hold is not counted repeatedly when operating on unique drawings.

**Scope:** `base` · **Level:** `pro` · **Status:** `not_started`.

**Evidence:** `proposal`.

## FBF-005 — Onion skin across layers

Compare references from other layers using visibility rules.

**Initial acceptance:** Locking a layer does not prevent using it as a reference.

**Scope:** `base` · **Level:** `pro` · **Status:** `not_started`.

**Evidence:** `proposal`.

## FBF-006 — Shift and trace

Temporarily move and rotate reference drawings for tracing.

**Initial acceptance:** Resetting the reference does not change source data.

**Scope:** `base` · **Level:** `pro` · **Status:** `not_started`.

**Evidence:** `proposal`.

## FBF-007 — Drawing desk

Arrange reference drawings independently of their timeline exposure.

**Initial acceptance:** Removing a desk reference does not delete its drawing.

**Scope:** `base` · **Level:** `pro` · **Status:** `not_started`.

**Evidence:** `proposal`.

## FBF-008 — Flip and short playback

Alternate poses and play a short segment without losing the selection.

**Initial acceptance:** Leaving short playback restores the working frame.

**Scope:** `base` · **Level:** `core` · **Status:** `partial`.

**Evidence:** `proposal`.

## FBF-009 — Drawing marks

Mark drawings as keys, breakdowns or custom categories.

**Initial acceptance:** A drawing mark appears on all its exposures.

**Scope:** `base` · **Level:** `pro` · **Status:** `not_started`.

**Evidence:** `proposal`.

## FBF-010 — Generate Color Art from lines

Build paint boundaries from Line Art using defined tolerances.

**Initial acceptance:** Regeneration offers a way to preserve manual corrections.

**Scope:** `base` · **Level:** `pro` · **Status:** `not_started`.

**Evidence:** `proposal`.

## FBF-011 — Generate matte

Produce reference silhouettes from drawings.

**Initial acceptance:** The silhouette preserves holes according to the configured option.

**Scope:** `base` · **Level:** `pro` · **Status:** `not_started`.

**Evidence:** `proposal`.

## FBF-012 — Registration and alignment crosses

Use alignment marks to register drawings.

**Initial acceptance:** Reference crosses are not rendered as final artwork.

**Scope:** `base` · **Level:** `pro` · **Status:** `not_started`.

**Evidence:** `proposal`.
