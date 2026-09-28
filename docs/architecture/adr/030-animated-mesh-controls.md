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
the whole chain. Each posed bone preserves its rest length. A vertex blends
the two rigid segment transforms by its saved weight. The elbow joint maps to
one point under both transforms. Zero-length segments, out-of-range weights
and nonfinite angles reject at the document boundary.

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
Scene-wide frame insertion/removal and selected-range Clear include deformer
keys. Clipboard paste, range move and stretch reject when those keys are in
scope until full per-substitution key transfer is implemented.

No external numerical library is needed for this bounded two-segment/cubic
evaluation. Eigen stays a candidate for heavier constrained solvers; adoption
requires a pinned revision, adapter boundary, workload comparison, license
inventory and fallback as specified in the library register.
