# AIX — Optional AI assistance

[Back to catalog](../README.md)

> Generated from `features.json` and `domains.json`; do not edit manually.

**Workflow:** Select → configure provider → generate → compare → accept.

**Module:** `ai-adapters`.

**Entities:** Provider, InferenceJob, Mask, GeneratedVariant.

**Relationships:** RAS, PRJ.

**Main risk:** Service dependencies, model licenses and loss of creative control.

## Shared contract

Mutations must respect transactions, undo/redo and persistence. Visual aids are not exported. Errors, cancellation and unsupported data must preserve the last valid state. These OPEN-TOON conditions will be specified per operation during implementation.

## AIX-001 — Optional enablement

Isolate AI features so the entire core editor works without them.

**Initial acceptance:** Disabling AI does not prevent opening, drawing or exporting projects.

**Scope:** `optional_extension` · **Level:** `optional` · **Status:** `not_started`.

**Evidence:** `proposal`.

## AIX-002 — AI masks

Create, modify and delete masks with controlled expansion.

**Initial acceptance:** The operation affects only the mask and selected padding.

**Scope:** `optional_extension` · **Level:** `optional` · **Status:** `not_started`.

**Evidence:** `proposal`.

## AIX-003 — Assisted erasing

Propose removal of masked content without destroying the original.

**Initial acceptance:** Rejecting a variant restores the input image exactly.

**Scope:** `optional_extension` · **Level:** `optional` · **Status:** `not_started`.

**Evidence:** `proposal`.

## AIX-004 — Image expansion

Generate content beyond image boundaries while keeping the original identifiable.

**Initial acceptance:** The original image retains registration after canvas expansion.

**Scope:** `optional_extension` · **Level:** `optional` · **Status:** `not_started`.

**Evidence:** `proposal`.

## AIX-005 — Resolution enhancement

Propose higher-resolution images or textures as variants.

**Initial acceptance:** The result reports the scale factor and retains a copy of the input.

**Scope:** `optional_extension` · **Level:** `optional` · **Status:** `not_started`.

**Evidence:** `proposal`.

## AIX-006 — Generative fill

Generate text-guided content within a region with manual acceptance.

**Initial acceptance:** Generation does not replace the resource until accepted.

**Scope:** `optional_extension` · **Level:** `optional` · **Status:** `not_started`.

**Evidence:** `proposal`.
