# OUT — Preview, rendering and export

[Back to catalog](../README.md)

> Generated from `features.json` and `domains.json`; do not edit manually.

**Workflow:** Preview → set range and quality → render → verify.

**Module:** `render`.

**Entities:** RenderJob, RenderProfile, FrameBuffer, OutputSpec.

**Relationships:** NOD, CAM, AUD.

**Main risk:** Preview/output differences, encoder failures and memory exhaustion.

## Shared contract

Mutations must respect transactions, undo/redo and persistence. Visual aids are not exported. Errors, cancellation and unsupported data must preserve the last valid state. These OPEN-TOON conditions will be specified per operation during implementation.

## OUT-001 — Interactive playback

Play at the scene frame rate with range, loop and performance status.

**Initial acceptance:** The counter distinguishes complete playback from dropped frames.

**Scope:** `base` · **Level:** `core` · **Status:** `not_started`.

**Evidence:** `proposal`.

## OUT-002 — Quality-controlled preview

Separate fast preview from final evaluation and show active simplifications.

**Initial acceptance:** Users can identify effects omitted from preview.

**Scope:** `base` · **Level:** `pro` · **Status:** `not_started`.

**Evidence:** `proposal`.

## OUT-003 — Preview timeout

Limit interactive work and flag recoverable incomplete results.

**Initial acceptance:** A heavy scene still accepts cancellation or completion of the frame.

**Scope:** `base` · **Level:** `pro` · **Status:** `not_started`.

**Evidence:** `proposal`.

## OUT-004 — Image sequences

Export range, name, numeric padding, resolution and alpha.

**Initial acceptance:** An inclusive range from 1 to 24 creates exactly 24 files.

**Scope:** `base` · **Level:** `core` · **Status:** `not_started`.

**Evidence:** `proposal`.

## OUT-005 — Video with audio

Render and encode using platform-available codec profiles.

**Initial acceptance:** Video and the audio mix begin at the same instant.

**Scope:** `base` · **Level:** `core` · **Status:** `not_started`.

**Evidence:** `proposal`.

## OUT-006 — Multiple outputs

Configure outputs with independent resolutions and formats.

**Initial acceptance:** One scene produces two outputs without changing global camera parameters.

**Scope:** `base` · **Level:** `pro` · **Status:** `not_started`.

**Evidence:** `proposal`.

## OUT-007 — Batch rendering

Run jobs outside the UI with logs, exit codes and retry support.

**Initial acceptance:** Retrying a failed frame does not unnecessarily rewrite valid frames.

**Scope:** `base` · **Level:** `pro` · **Status:** `not_started`.

**Evidence:** `proposal`.

## OUT-008 — Output crop and scale

Crop and resize in output nodes using a defined filter.

**Initial acceptance:** Cropping preserves the specified origin and dimensions.

**Scope:** `base` · **Level:** `pro` · **Status:** `not_started`.

**Evidence:** `proposal`.

## OUT-009 — PSD layout and layers

Export layouts while preserving layers supported by the profile.

**Initial acceptance:** Reopening the test PSD preserves registration and transparency.

**Scope:** `base` · **Level:** `pro` · **Status:** `not_started`.

**Evidence:** `proposal`.

## OUT-010 — After Effects interchange

Document profile-based export of data and per-layer renders.

**Initial acceptance:** The recipient receives a report of baked effects.

**Scope:** `base` · **Level:** `advanced` · **Status:** `not_started`.

**Evidence:** `proposal`.

## OUT-011 — GIF and lightweight output

Generate preview animations with explicit palette, loop and format limits.

**Initial acceptance:** GIF duration matches within the format's timing granularity.

**Scope:** `base` · **Level:** `pro` · **Status:** `not_started`.

**Evidence:** `proposal`.

## OUT-012 — Sequence player

Play rendered results with audio without opening the full editor.

**Initial acceptance:** Missing sequence frames are clearly flagged.

**Scope:** `base` · **Level:** `pro` · **Status:** `not_started`.

**Evidence:** `proposal`.

## OUT-013 — Output precision and color

Choose bit depth, alpha and color space per output.

**Initial acceptance:** A float EXR preserves values above one in the HDR profile.

**Scope:** `base` · **Level:** `advanced` · **Status:** `not_started`.

**Evidence:** `proposal`.
