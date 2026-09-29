# ADR-032 — Named masked character poses

Status: working HM-07 subset, 2026-09-29.

## Contract

Format 12 adds named `CharacterPose` records to a Character root. A pose has a
stable ID and one or more `PosePart` entries. Each entry identifies a Part in
that same character, an explicit channel bit mask, an evaluated local
transform captured at a scene frame, and an optional registered drawing ID.
The mask covers eight transform channels and one discrete drawing channel.
Unmasked drawing IDs are zero. Names are unique within the character; pose
IDs use the document allocator. Bounds, ownership, transforms, masks and
substitution references are validated before publication or load.

Capture samples the selected frame, which makes an interpolated pose reusable.
Apply creates full-pose transform keys only for included numeric channels and
sets a held exposure only where the drawing bit is present. All other channel
values are sampled at the destination frame and retained. Application edits a
candidate document and validates it before replacing the caller's document;
the Session command creates one undo entry. A locked target rejects the whole
operation. Editing a saved pose's name or deleting it also uses Session.

The Properties panel offers capture for the selected Part or all Parts and a
visible channel-group choice. A saved pose can be selected, applied, renamed
or removed at the playhead. Its slider interpolates numeric channels from the
gesture's initial evaluated pose. Zero leaves the document unchanged; one
reaches the stored numeric and drawing endpoints exactly. A drawing selection
changes at amount 0.5 and remains held through one. During a drag, Session
recomputes each candidate from the same baseline, publishes each validated
preview and coalesces the gesture into one undo entry. Other edits end the
gesture. The domain model allows different masks for each Part; the current
capture UI applies one chosen mask to the target set.
Rig can later set or replace the selected Part's captured values and mask in
that pose, or remove that Part while at least one mapping remains. This gives
each Part an independent mask without requiring a separate pose asset.

Character duplication remaps pose, Part and drawing IDs into the independent
copy. Branch deletion and Part detachment remove affected entries and empty
poses. A substitution referenced by a drawing-masked pose cannot be removed
until that pose changes or is removed. Format 11 and older projects load with
an empty pose collection; the first save in format 12 keeps the existing
source-version backup policy. A format-12 file cannot be opened by an older
editor without restoring its backup.

The bounded cross-character transfer copies a saved pose to another Character
in the same document. It maps Parts by roles that must be unique in both
Characters, and drawing choices by names that must be unique in both mapped
Parts. Their local rest transforms must match exactly; incompatible rigs
reject before mutation. The destination receives a new pose ID, its own Part
and drawing IDs, and an unpublished copy. A name collision receives a numeric
suffix. Transfer does not change either character's current artwork or keys.
The Rig inspector offers a compact destination selector and **Copy to**
command; a successful copy selects the destination pose.

## Verification and limits

Domain tests cover mixed masks, blend endpoints and the drawing threshold,
excluded values, coalesced undo/redo, independent character copy and
stale-entry cleanup. They also reject a drawing-channel capture without an
exposure and removal of the final Part entry without mutating the document.
Storage tests cover format-12 SQLite/JSON round trips and format-11 loading.
The controller test imports original registered PNG parts, captures a selected
Part, applies it at another frame, undoes/redoes and reopens the project.

Transfer tests apply a remapped pose, preserve the source, reject role/drawing
and rest-transform mismatches atomically, undo/redo and save/reopen. Native Qt
Quick smoke checks the destination control in the Rig inspector on a copied
15-Part character and records `build/hm07-transfer-smoke.png`.

Cross-project transfer, incompatible rest retargeting, mirroring and deformer
channels remain HM-07/HM-09 work. This ADR does not close RIG-012 or HM-07.
