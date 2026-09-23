# IMP — Import, scanning and interchange

[Back to catalog](../README.md)

> Generated from `features.json` and `domains.json`; do not edit manually.

**Workflow:** Import → verify conversions → link resources → correct losses.

**Module:** `interchange`.

**Entities:** ImportJob, FormatAdapter, ConversionReport, ExternalAsset.

**Relationships:** PRJ, LYR.

**Main risk:** A filename extension does not imply fidelity for every format feature.

## Shared contract

Mutations must respect transactions, undo/redo and persistence. Visual aids are not exported. Errors, cancellation and unsupported data must preserve the last valid state. These OPEN-TOON conditions will be specified per operation during implementation.

## IMP-001 — Images and sequences

Import still images or sequences with explicit alpha, ordering and exposure.

**Initial acceptance:** A sequence with gaps reports how they are resolved.

**Scope:** `base` · **Level:** `core` · **Status:** `partial`.

**Specification source:** `proposal`.

**Implementation evidence:** [docs/implementation/status.json](../../../docs/implementation/status.json) — experimental subset only, catalog acceptance not closed.

**Remaining scope:** The proposed behavior and acceptance test are OPEN-TOON requirements, not a claim of implementation.

## IMP-002 — Layered PSD

Read hierarchy, opacity and supported modes while reporting losses.

**Initial acceptance:** Unsupported modes are reported before accepting conversion.

**Scope:** `base` · **Level:** `pro` · **Status:** `not_started`.

**Specification source:** `proposal`.

## IMP-003 — PSD layout

Reuse layout composition and positions at a known scale.

**Initial acceptance:** Layers retain their relative registration after import.

**Scope:** `base` · **Level:** `pro` · **Status:** `not_started`.

**Specification source:** `proposal`.

## IMP-004 — External vectors

Convert compatible vector formats while preserving supported curves, fills and transforms.

**Initial acceptance:** The report lists filters or fonts that cannot be transferred.

**Scope:** `base` · **Level:** `pro` · **Status:** `not_started`.

**Specification source:** `proposal`.

## IMP-005 — Reference video

Import video for reference or frame extraction with an explicit clock.

**Initial acceptance:** A variable-rate clip is normalized without hidden drift.

**Scope:** `base` · **Level:** `pro` · **Status:** `not_started`.

**Specification source:** `proposal`.

## IMP-006 — Link external image

Retain a link and allow content updates from disk.

**Initial acceptance:** Updating content does not erase the layer transform.

**Scope:** `base` · **Level:** `pro` · **Status:** `not_started`.

**Specification source:** `proposal`.

## IMP-007 — Scanning

Acquire drawings with resolution, registration and numbering order.

**Initial acceptance:** Rescanning a sheet can replace its drawing without changing exposures.

**Scope:** `base` · **Level:** `advanced` · **Status:** `not_started`.

**Specification source:** `proposal`.

## IMP-008 — Vectorization

Convert scans to strokes or regions using repeatable parameters.

**Initial acceptance:** The same preset and input produce the same output.

**Scope:** `base` · **Level:** `advanced` · **Status:** `not_started`.

**Specification source:** `proposal`.

## IMP-009 — Legacy FLA and SWF

Document assisted conversion and limits of older formats.

**Initial acceptance:** Unconvertible features appear in a report instead of disappearing silently.

**Scope:** `legacy_candidate` · **Level:** `legacy` · **Status:** `not_started`.

**Specification source:** `proposal`.

## IMP-010 — Editorial interchange

Receive layouts or animatics as scenes with timing and resources.

**Initial acceptance:** Imported shot duration matches the reference animatic.

**Scope:** `base` · **Level:** `pro` · **Status:** `not_started`.

**Specification source:** `proposal`.
