# ADR-044 — Persistent cutter bypass

Status: experimental HM-12 subset, 2026-09-29.

## Decision

Format 20 adds `matteBypassed` to Drawing and Part layers with a cutter
binding. It defaults to false for formats 1–19. Bypass retains the cutter
source ID and Inside/Outside choice, while the target is composited at its
uncut alpha. The cutter source remains reserved and does not paint into the
final composite. Re-enabling the binding restores its prior coverage without
reselecting the source.

The derived graph represents bypass as an Image → Bypassed cutter → Image
node. It omits the inactive matte input from the Write/Display dependency
chain, so a cutter-source edit cannot invalidate the target output while
bypassed. The UI offers the same command in Properties and Nodes. Commands
use the document transaction path; removing a binding clears bypass and
inversion. A new source starts enabled, while reselecting the same source
preserves its state. Referenced-source deletion remains guarded even if the
binding is bypassed.

The first format-20 save of an older project keeps a readable source-version
backup, including `.pre-v19.bak`. Invalid bypass without a source rejects
at the document boundary.

## Evidence and limits

The render fixture compares fractional Inside, bypassed and restored pixels,
and confirms that the cutter does not paint in bypass. Graph tests check the
typed identity node, dependency isolation and invalid-state rejection. The
format-19 migration test checks default-off bypass, save/reopen and backup.
Native Qt Quick smoke edits, undoes, redoes, saves and reopens a bypassed
cutter, then re-enables it. General editable node bypass and the broader
node graph remain open.
