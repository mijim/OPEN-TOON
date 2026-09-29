# ADR-038 — Format-17 nondestructive audio repeats

Status: experimental HM-10 subset, 2026-09-29.

## Contract

Format 17 adds `repeats` to each audio clip, an integer from 1 through 64.
Formats 1–16 load with one repeat; the first format-17 save of an older file
creates a readable source-version backup, for example `.pre-v16.bak`. The
original WAV resource and its trim range remain unchanged. Editing repeats
is one transactional command with undo/redo and validation at both the
command and document-load boundaries.

Let `L = outSample - inSample`. For each output sample at or after the clip's
scene start, convert its elapsed time to an integer source-sample offset with
the existing rational mapping. Play only offsets in `[0, L × repeats)` and
read source sample `inSample + offset mod L`. Linear interpolation crosses an
internal repeat seam into `inSample`; at the final end it holds the last
sample before silence. This keeps repeats contiguous in source time even at
24000/1001 fps and when source and output sample rates differ.

The visible waveform and clip end use that same source-sample offset. A
single frame may cross multiple repeat seams; its peak query combines exact
partial intervals and a whole-loop maximum if needed. The visible end is
the first scene frame whose sample offset reaches `L × repeats`, clamped to
scene duration. The Audio panel exposes the repeat count; clip placement
still starts on a whole frame.

## Evidence and limits

Tests render three exact cues from one trimmed source, compare waveform
peaks on each loop, reject an invalid repeat count atomically, undo and
reopen a repeated clip, migrate version-16 JSON with default one repeat and
verify a readable `.pre-v16.bak` after saving. The native Qt Quick audio
smoke checks a repeated cue in both waveform and WAV export.

The preview still uses linear source-rate conversion. Long hardware playback,
device queue latency and audiovisual drift remain HM-10 qualification gates.
