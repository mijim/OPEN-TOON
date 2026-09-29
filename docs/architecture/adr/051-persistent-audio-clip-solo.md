# ADR-051 — Persistent audio clip solo

Status: experimental HM-10 subset, 2026-09-29.

## Decision

Each placed audio clip stores an independent `solo` flag in project format 26.
When no clip is soloed, every unmuted clip contributes to the mix. When one or
more clips are soloed, only unmuted soloed clips contribute. Mute always takes
precedence over solo. The rule is evaluated once when an immutable mix plan
is built, so preview, scrub and WAV export use the same selection without a
document lock in the callback. The timeline retains but dims excluded clips.

The Audio panel toggles Solo and Unsolo through one transactional command.
Duplicate and split copy the flag with the clip; the original WAV bytes and
source references do not change. Format-25 clips load unsoloed. A first save
to format 26 keeps a readable `.pre-v25.bak` project. Current-format files
missing the flag reject before publication.

## Evidence and limits

Two independent cue clips verify one- and two-solo mixing, mute precedence,
undo/redo, failed-command rollback and exact PCM after save/reopen. A migration
test checks the format-25 default, backup and malformed format-26 rejection.
Native Qt Quick smoke clicks Solo and undoes it. Solo is clip scoped; grouped
tracks and audio routing remain outside this bounded subset.
