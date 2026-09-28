# ADR-030 — Animated bone and curve controls over rest meshes

Status: HM-06 implementation in progress, 2026-09-28. Acceptance requires
native control interaction, complete bent-arm/curved-torso fixture, quality
and cost evidence, and save/reopen/export parity.

## Proposed bounded contract

Format 9 extends one Part/substitution mesh binding with at most one deformer:
a two-segment forward-kinematic bone chain or a cubic curve. Controls and
weights are drawing-local. The saved rest mesh/UV remains the texture source;
evaluation writes only an ephemeral posed mesh for one frame. A static HM-05
pose must be reset before assigning a deformer, and static vertex editing is
rejected while one is attached.

The bone profile stores three rest joints, one elbow transition radius,
per-vertex distal weights and keyed two-angle poses. The root joint stays at
its drawing-local rest coordinate for this subset; Part/Peg transforms move
the whole chain. Each posed bone preserves its rest length. A vertex uses its
saved weight to interpolate segment rotation around the shared posed elbow,
preserving its rest distance from that joint and reducing mid-influence
pinching. The elbow joint maps to one point for every weight. Zero-length
segments, out-of-range weights and nonfinite angles reject at the document
boundary. In Rest joints mode, direct canvas drags may reposition any of the
three drawing-local rest joints. The transaction recomputes distal weights
from the unchanged rest mesh, validates existing keyed and intermediate poses,
then publishes the new chain atomically. A rejected move leaves the previous
binding, keys and pixels intact. The root remains fixed while posing; Rest
joints mode changes its stored setup position without changing rest artwork.
The elbow transition radius is editable in drawing-local pixels through an
undoable use case. Rest joints mode draws a dashed influence circle and a
square handle along the normal of the proximal segment. Dragging that handle
projects the pointer into drawing-local space, previews on an isolated
candidate and commits one undoable edit; Escape discards the preview. Changing
the radius recomputes normalized vertex weights while
leaving rest UVs and authored angle keys intact; existing keys and sampled
intermediate poses must still pass fold and proxy checks. The initial radius
considers both chain length and cross-section width so a fine grid on a broad
image does not start with a needlessly narrow blend. The selected substitution's
mesh grid can be bound with one to 32 cells on each axis through Properties;
the saved topology is displayed there and cannot be silently replaced while
a deformer is attached.

The curve profile stores four rest cubic controls, a fixed per-vertex curve
parameter and keyed four-point posed controls. Each vertex receives the
posed-minus-rest cubic displacement at its saved parameter. Moving a tangent
changes the field continuously without rotating cross sections; this bounded
choice avoids the foldover observed when wide cross sections used unrestricted
normal-frame transport. Authored and intermediate poses still require positive
triangle orientation; no NaNs or implicit topology regeneration are allowed.

Pose keys use the document's integer frame model and deterministic
linear/held/smoothstep interpolation. A first later-frame key anchors the unchanged
rest control state at frame zero. There is no second animation clock.
Evaluation occurs after document pose keys and before camera/layer image
composition. Immutable render snapshots carry the authored keys; revision
cache keys already cover new document edits. Independent Part duplication
copies weights and keys, while linked artwork may have a separate deformer.
The canvas mesh grid samples the same evaluated pose as artwork, including
the temporary drag preview; the rest grid is available for static mesh setup
and bone rest-joint placement. Curve rest controls are also directly editable
after binding: changing one rest control shifts the corresponding authored
control in every key by the same delta, preserving each key's offset from
rest. Vertex parameters are recalculated from the unchanged rest mesh, and
all keys and intermediate samples are validated before atomic publication.
The drawing's frame-zero pixels remain unchanged. The native canvas drag uses
an isolated preview and the same undoable command as other deformer edits.

The profile binds one two-segment chain to one source image. A continuous arm
or leg should therefore be authored as **one image and one mesh**, with three
anatomical rest joints; a separately pivoted upper/lower image pair does not
exercise continuous elbow/knee deformation. The original 19-part fixture
remains useful for registered intake and substitutions. A 15-artwork-Part
working rig combines each upper/lower limb pair into four single images. The
separate hands and feet use the bounded format-10 parent bone-tip link defined
in [ADR-031](031-bone-tip-attachment.md). Broader attachment and IK work
remains in HM-09.
Scene-wide frame insertion/removal and selected-range Clear include deformer
keys. Same-Part range copy/paste, move and stretch preserve the keyed bone or
curve identity and exact pose endpoints. A cross-scene or different-Part key
paste rejects before mutation until portable rig binding transfer exists.
Independent drawing paste also rejects a bound Part because cloning its source
without a matching mesh would change the rendered result.

No external numerical library is needed for this bounded two-segment/cubic
evaluation. Eigen stays a candidate for heavier constrained solvers; adoption
requires a pinned revision, adapter boundary, workload comparison, license
inventory and fallback as specified in the library register.
