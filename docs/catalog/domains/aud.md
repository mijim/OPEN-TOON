# AUD — Sound and lip sync

[Back to catalog](../README.md)

> Generated from `features.json` and `domains.json`; do not edit manually.

**Workflow:** Import sound → adjust → listen → detect and correct mouths.

**Module:** `audio`.

**Entities:** AudioAsset, AudioClip, AudioTrack, PhonemeTrack, VisemeMap.

**Relationships:** TIM, RIG.

**Main risk:** Audiovisual drift, audio dropouts and imperfect detection.

## Shared contract

Mutations must respect transactions, undo/redo and persistence. Visual aids are not exported. Errors, cancellation and unsupported data must preserve the last valid state. These OPEN-TOON conditions will be specified per operation during implementation.

## AUD-001 — Import sound

Decode supported formats while preserving the original sample and metadata.

**Initial acceptance:** An incompatible file reports an error without leaving an empty track.

**Scope:** `base` · **Level:** `core` · **Status:** `partial`.

**Specification source:** `proposal`.

**Implementation evidence:** [docs/implementation/HM10-PROGRESS.md](../../../docs/implementation/HM10-PROGRESS.md) — bounded mono/stereo PCM16 WAV import preserves original bytes and rejects incompatible input atomically.

**Remaining scope:** The proposed behavior and acceptance test are OPEN-TOON requirements, not a claim of implementation.

## AUD-002 — Waveform

Display amplitude at different timeline zoom levels.

**Initial acceptance:** The reference peak aligns with its audio sample.

**Scope:** `base` · **Level:** `core` · **Status:** `partial`.

**Specification source:** `proposal`.

**Implementation evidence:** [docs/implementation/HM10-PROGRESS.md](../../../docs/implementation/HM10-PROGRESS.md) — sample-aligned per-frame waveform peaks in timeline at current zoom, no pyramid cache or audio playback.

**Remaining scope:** The proposed behavior and acceptance test are OPEN-TOON requirements, not a claim of implementation.

## AUD-003 — Trim and place clips

Move clip start, adjust in/out points and repeat clips nondestructively.

**Initial acceptance:** Trimming and undoing restores the complete audio.

**Scope:** `base` · **Level:** `core` · **Status:** `partial`.

**Specification source:** `proposal`.

**Implementation evidence:** [docs/implementation/HM10-PROGRESS.md](../../../docs/implementation/HM10-PROGRESS.md) — undoable clip start and half-open source-sample trim, no repeat.

**Remaining scope:** The proposed behavior and acceptance test are OPEN-TOON requirements, not a claim of implementation.

## AUD-004 — Track mixing

Control volume and play multiple synchronized tracks.

**Initial acceptance:** Aligned tracks mix without a time offset.

**Scope:** `base` · **Level:** `core` · **Status:** `partial`.

**Specification source:** `proposal`.

**Implementation evidence:** [docs/implementation/HM10-PROGRESS.md](../../../docs/implementation/HM10-PROGRESS.md) — saved clip gain control only, synchronized playback/mixing remains open.

**Remaining scope:** The proposed behavior and acceptance test are OPEN-TOON requirements, not a claim of implementation.

## AUD-005 — Scrubbing

Play audio fragments while traversing frames, including continuous scrubbing.

**Initial acceptance:** Dragging to the marked frame plays the expected fragment.

**Scope:** `base` · **Level:** `pro` · **Status:** `not_started`.

**Specification source:** `proposal`.

## AUD-006 — Lip-sync detection

Derive candidate phonemes or visemes from audio with editable results.

**Initial acceptance:** Detection does not overwrite manual corrections without explicit selection.

**Scope:** `base` · **Level:** `pro` · **Status:** `not_started`.

**Specification source:** `proposal`.

## AUD-007 — Mouth mapping

Associate detected labels with character drawings.

**Initial acceptance:** Labels without an assigned drawing are flagged.

**Scope:** `base` · **Level:** `pro` · **Status:** `not_started`.

**Specification source:** `proposal`.

## AUD-008 — Manual lip-sync correction

Edit mouth drawings and duration independently of the detector.

**Initial acceptance:** Manual corrections survive saving and reopening.

**Scope:** `base` · **Level:** `core` · **Status:** `not_started`.

**Specification source:** `proposal`.

## AUD-009 — Audio library

Find, preview and reuse scene audio without unnecessary duplication.

**Initial acceptance:** Renaming a library entry does not move the source file.

**Scope:** `base` · **Level:** `pro` · **Status:** `not_started`.

**Specification source:** `proposal`.

## AUD-010 — Export mix

Export audio by range and synchronize it with rendering.

**Initial acceptance:** Exported duration matches the rational scene range.

**Scope:** `base` · **Level:** `pro` · **Status:** `not_started`.

**Specification source:** `proposal`.
