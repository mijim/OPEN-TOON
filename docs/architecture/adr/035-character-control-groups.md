# ADR-035 — Published character control groups

Status: working HM-07 subset, 2026-09-29.

## Contract

Format 15 adds a `controlGroup` string to each saved character pose, view set
and named drawing substitution. The default is `Main`; formats 14 and older
load with that value. Group names must contain 1–64 UTF-8 bytes. Changing a
group is an undoable document command. Character duplication retains each
group while assigning independent Part, drawing, pose and view IDs. The first
format-15 save of an older project preserves a backup named for its source
version, including `.pre-v14.bak`.

Rig exposes the group name beside each publication toggle. Animator collects
groups with at least one published control and shows one group at a time in
both the side dashboard and the compact camera viewport panel. Selecting a
group changes local view state only: it does not edit the document, frame,
selection or output pixels. A view, pose or drawing from another group cannot
be invoked through Animator's controller actions. Published controls continue
to use the existing transactional commands, and no persistent graph driver or
dependency edge is created.

The group string is descriptive metadata rather than a new control graph.
It limits which direct commands are visible and applicable in Animator. Any
future simultaneous driver system requires typed writes and conflict/cycle
validation before it can replace this bounded command model.

## Evidence and limits

Domain tests verify group validation, one-step undo/redo, independent copies
and unchanged rendering. Storage tests migrate a format-14 fixture, save the
three group types, reopen and load its `.pre-v14.bak` backup. Editor tests
switch Face/Body/Stage groups without changing scene state and reject hidden
pose, view and drawing commands. Native Qt Quick smoke checks both group
pickers, the Face drawing panel, Body pose, Stage-only view and unchanged
output; `build/hm07-groups-smoke.png` was visually inspected.

Groups do not provide user-positioned control widgets, driver combinations,
automatic dependency resolution or reusable rig templates.
