# ADR-033 — Published character controls and Animator view

Status: working HM-07 subset, 2026-09-29.

## Contract

Format 13 adds a `published` boolean to saved character poses and coordinated
view sets. It defaults to false when formats 12 and older load. Publishing is
an undoable document command, scoped to the owning Character. Duplicating a
Character remaps pose, view, Part and drawing IDs; the copy retains each
publication choice while its bindings remain independent. Publication is
metadata and never enters the renderer. The first format-13 save of an older
project preserves the source-version backup, including `.pre-v12.bak`.

The Rig workspace exposes all editable pose and view records, including their
publication toggles. The Animator workspace shows only published view buttons
and a bounded 0–1 pose slider. View buttons call the existing transactional
view command. The pose slider calls the format-12 masked blend command and
coalesces its drag into one undo. The widget cannot target a Part, drawing or
channel outside its saved pose/view mapping. The workspace mode is local view
state stored with Qt settings; switching modes retains document selection and
frame and changes no render pixels. The same saved document is used in both
workspaces.

These are direct commands, not persistent graph drivers: only the active
gesture writes the candidate document. No dependency edge or expression is
created, so there is no control graph cycle to evaluate. Simultaneous
published-control drivers, general on-canvas widget bindings and conflict
resolution remain outside this subset. Adding those requires an explicit
typed driver contract and cycle validation before HM-07 can be accepted.

## Verification and limits

Domain tests check publication leaves rendered pixels unchanged and a copied
character receives independent bound IDs. Storage tests load format 12 with
publication off and verify a readable `.pre-v12.bak` after upgrading. The
editor integration test switches workspaces without changing selection, frame
or pixels, then saves and reopens the publication flags. The native Qt Quick
dashboard smoke opens the original continuous-character project, publishes a
view and a pose, checks visible controls and captures the actual window. It
also measures a real mouse drag to `frameSwapped` and validates one-step undo;
three 40-sample runs passed the proposed sampled interaction budget on the
recorded M1 Pro host.

The two workspace modes do not yet provide saved geometry/docking presets for
drawing, compositing and other phase work. On-canvas control widgets, control
groups/switches, conflict handling and character portability remain open.
