# TIM — Timeline, Xsheet and exposure

[Back to catalog](../README.md)

> Generated from `features.json` and `domains.json`; do not edit manually.

**Workflow:** Expose → adjust timing → play → review sheet.

**Module:** `timeline`.

**Entities:** FrameIndex, RationalRate, ExposureSpan, Marker, Annotation.

**Relationships:** LYR.

**Main risk:** A one-frame offset affects sound, drawing and rendering.

## Shared contract

Mutations must respect transactions, undo/redo and persistence. Visual aids are not exported. Errors, cancellation and unsupported data must preserve the last valid state. These OPEN-TOON conditions will be specified per operation during implementation.

## TIM-001 — Synchronized timeline and Xsheet

Provide horizontal and vertical views of the same time model.

**Initial acceptance:** An Xsheet edit appears immediately in the timeline.

**Scope:** `base` · **Level:** `core` · **Status:** `partial`.

**Specification source:** `proposal`.

**Implementation evidence:** [docs/implementation/status.json](../../../docs/implementation/status.json) — experimental subset only, catalog acceptance not closed.

**Remaining scope:** The proposed behavior and acceptance test are OPEN-TOON requirements, not a claim of implementation.

## TIM-002 — Create drawings and exposures

Create unique drawings and reference them from one or more frame intervals.

**Initial acceptance:** Two cells referencing the same drawing reflect a shared edit.

**Scope:** `base` · **Level:** `core` · **Status:** `partial`.

**Specification source:** `proposal`.

**Implementation evidence:** [docs/implementation/status.json](../../../docs/implementation/status.json) — experimental subset only, catalog acceptance not closed.

**Remaining scope:** The proposed behavior and acceptance test are OPEN-TOON requirements, not a claim of implementation.

## TIM-003 — Hold and extend exposures

Lengthen or shorten holds without duplicating drawing data.

**Initial acceptance:** Extending a hold by ten frames does not create ten drawings.

**Scope:** `base` · **Level:** `core` · **Status:** `partial`.

**Specification source:** `proposal`.

**Implementation evidence:** [docs/implementation/status.json](../../../docs/implementation/status.json) — experimental subset only, catalog acceptance not closed.

**Remaining scope:** The proposed behavior and acceptance test are OPEN-TOON requirements, not a claim of implementation.

## TIM-004 — Insert and remove frames

Change duration by shifting ranges according to an explicit rule.

**Initial acceptance:** Inserting two frames shifts markers and content according to the selected option.

**Scope:** `base` · **Level:** `core` · **Status:** `partial`.

**Specification source:** `proposal`.

**Implementation evidence:** [docs/implementation/status.json](../../../docs/implementation/status.json) — experimental subset only, catalog acceptance not closed.

**Remaining scope:** The proposed behavior and acceptance test are OPEN-TOON requirements, not a claim of implementation.

## TIM-005 — Clear exposure

Remove a time reference while keeping the drawing available.

**Initial acceptance:** The drawing can be exposed again from the library.

**Scope:** `base` · **Level:** `core` · **Status:** `partial`.

**Specification source:** `proposal`.

**Implementation evidence:** [docs/implementation/status.json](../../../docs/implementation/status.json) — experimental subset only, catalog acceptance not closed.

**Remaining scope:** The proposed behavior and acceptance test are OPEN-TOON requirements, not a claim of implementation.

## TIM-006 — Copy and paste ranges

Distinguish pasting exposures, drawings, keyframes and combinations.

**Initial acceptance:** Pasting exposures does not add transform keyframes.

**Scope:** `base` · **Level:** `core` · **Status:** `partial`.

**Specification source:** `proposal`.

**Implementation evidence:** [docs/implementation/status.json](../../../docs/implementation/status.json) — experimental subset, complete catalog acceptance remains open.

**Remaining scope:** The proposed behavior and acceptance test are OPEN-TOON requirements, not a claim of implementation.

## TIM-007 — Repeats and cycles

Repeat sequences with shared drawings and an explicit time limit.

**Initial acceptance:** Correcting the original drawing corrects its linked repetitions.

**Scope:** `base` · **Level:** `core` · **Status:** `partial`.

**Specification source:** `proposal`.

**Implementation evidence:** [docs/implementation/status.json](../../../docs/implementation/status.json) — experimental subset, complete catalog acceptance remains open.

**Remaining scope:** The proposed behavior and acceptance test are OPEN-TOON requirements, not a claim of implementation.

## TIM-008 — Timing on twos and threes

Apply regular exposure steps while preserving drawing order.

**Initial acceptance:** Four drawings on twos occupy eight frames.

**Scope:** `base` · **Level:** `core` · **Status:** `partial`.

**Specification source:** `proposal`.

**Implementation evidence:** [docs/implementation/status.json](../../../docs/implementation/status.json) — experimental subset, complete catalog acceptance remains open.

**Remaining scope:** The proposed behavior and acceptance test are OPEN-TOON requirements, not a claim of implementation.

## TIM-009 — Drag, stretch and overwrite

Move blocks with controlled insertion and overwrite options.

**Initial acceptance:** The editor previews the range to be replaced before dropping.

**Scope:** `base` · **Level:** `pro` · **Status:** `partial`.

**Specification source:** `proposal`.

**Implementation evidence:** [docs/implementation/status.json](../../../docs/implementation/status.json) — experimental subset, complete catalog acceptance remains open.

**Remaining scope:** The proposed behavior and acceptance test are OPEN-TOON requirements, not a claim of implementation.

## TIM-010 — Navigate between drawings

Jump to a distinct drawing, keyframe or range boundary.

**Initial acceptance:** Within a hold, next drawing jumps to the next actual change.

**Scope:** `base` · **Level:** `core` · **Status:** `partial`.

**Specification source:** `proposal`.

**Implementation evidence:** [docs/implementation/status.json](../../../docs/implementation/status.json) — experimental subset only, catalog acceptance not closed.

**Remaining scope:** The proposed behavior and acceptance test are OPEN-TOON requirements, not a claim of implementation.

## TIM-011 — Scene markers

Create named markers and ranges with duration and notes.

**Initial acceptance:** A locked marker does not move when dragging the ruler.

**Scope:** `base` · **Level:** `pro` · **Status:** `partial`.

**Specification source:** `proposal`.

**Implementation evidence:** [docs/implementation/status.json](../../../docs/implementation/status.json) — experimental subset, complete catalog acceptance remains open.

**Remaining scope:** The proposed behavior and acceptance test are OPEN-TOON requirements, not a claim of implementation.

## TIM-012 — Frame marks

Classify keys, breakdowns and in-betweens using readable symbols.

**Initial acceptance:** Symbols remain distinguishable in monochrome.

**Scope:** `base` · **Level:** `pro` · **Status:** `not_started`.

**Specification source:** `proposal`.

## TIM-013 — Exposure thumbnails

Show cached drawing previews with selective updates.

**Initial acceptance:** Changing a drawing invalidates only its associated thumbnails.

**Scope:** `base` · **Level:** `pro` · **Status:** `not_started`.

**Specification source:** `proposal`.

## TIM-014 — Print Xsheet

Export or print sheets with columns, timing, numbering and annotations.

**Initial acceptance:** A multi-page sheet repeats headers and omits no frames.

**Scope:** `base` · **Level:** `pro` · **Status:** `partial`.

**Specification source:** `proposal`.

**Implementation evidence:** [docs/implementation/status.json](../../../docs/implementation/status.json) — experimental subset, complete catalog acceptance remains open.

**Remaining scope:** The proposed behavior and acceptance test are OPEN-TOON requirements, not a claim of implementation.
