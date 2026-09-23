# HM-01 persistent rig-property contract — accepted bounded evidence

The existing full-pose evaluator and curve commands now accept stable typed
Character, Peg and Part transform addresses. Rest/setup, an explicitly authored
key, and the evaluated value at a frame remain distinct queries. A mismatched
entity kind, missing ID, invalid numeric value, duplicate target or locked layer
is rejected before a `Session` command publishes its candidate. Existing
curve/key behavior remains covered by the animation, render and controller
tests; this contract does not claim independent channel keys.

Format 4 stores the first consumer semantics: character/peg/part identity,
part roles and named drawing alternatives. The migration test constructs a
format-3 revision, injects a failed upgrade, checks the old scene and
`.pre-v3.bak` remain readable, then upgrades and reopens the typed rig. The
format-1 and format-2 tests verify source-version backup names. Existing save
fault-injection, abrupt-termination, conflict, asset integrity and full-pose
round-trip tests still pass. An older format-3 reader rejects schema/JSON
version 4 by its version guard.

The 2026-09-23 macOS arm64 locked build passed 72 CTest entries and the native
mouse/animation smoke. `tests/rigging_tests.cpp` compares rendered pixels before
and after peg/pivot/reparent edits, checks typed-address mismatch, singular and
shear rejection, substitution holds, undo and serialization. The controller
workflow test assembles registered PNG parts and reopens the result. Physical
tablet input and full character animator acceptance remain separate later work.

HM-01 is a bounded dependency contract. It does not complete P01, P06 or P08.
See [ADR-025](../architecture/adr/025-character-rig-format-four.md) for the
format and current geometric limits.
