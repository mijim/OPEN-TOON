# ADR-034 — Published drawing substitutions

Status: working HM-07 subset, 2026-09-29.

## Contract

Format 14 adds a `published` flag to each named drawing substitution. Older
formats load with publication off. Publication is an undoable document edit and
does not affect evaluation or rendering. Independent Character duplication
retains the flag on remapped substitutions. The existing source-version backup
is written before the first format-14 save of an older project.

Rig exposes **Show in Animator** on the selected substitution. Animator groups
published drawing choices by Part role. Selecting a choice changes only that
Part's held drawing at the current frame, through the same transactional
substitution command used in Rig. The command verifies that the Part belongs
to the current Character and the drawing is a published option for that Part.
It cannot change another Part or create a persistent driver. The panel remains
a compact document control rather than an independent scene graph.

## Evidence and limits

Domain tests cover independent copy and unchanged rendering. Storage tests
cover format-13 migration, publication defaults and the `.pre-v13.bak` source
backup. Editor integration switches a published hand choice, checks every
other Part and existing keys, then undoes, redoes and reopens. The native
Animator smoke renders a continuous character with published mouth choices,
checks the visible QML group, switches the mouth and undoes it.

General on-canvas controls, persistent multi-driver dependencies, conflict
resolution and portable cross-character controls remain HM-07 work.
