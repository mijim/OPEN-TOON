# ADR-029 — Per-substitution rest mesh and format-8 binding

Status: accepted bounded HM-05 engineering contract, 2026-09-28. See
[verification](../../implementation/HM05-ACCEPTANCE.md). P09 remains open.

## Decision

Format 8 adds optional mesh bindings to a Part. Each binding addresses one
registered drawing ID and records that image's source dimensions, a bounded
regular-grid topology, explicit drawing-local rest coordinates, normalized UVs
and a separately editable static pose preview. Its drawing ID belongs to the
Part, so a linked drawing may have independent bindings in two characters.
The source image remains the editable, immutable artwork. The mesh does not
replace its pixels or alter the drawing's vector/raster data.

The rest coordinates and UVs are saved intent; pose coordinates are an
explicit temporary HM-05 preview state. Resetting the mesh pose copies rest
coordinates without changing layer transform keys. HM-06 will define animated
deformer controls and their evaluated mesh pose separately; this preview is
not represented as a bone or curve animation track.

Commands generate a regular grid for a supported image or vector substitution, edit
one pose or rest vertex, reset the pose or remove a binding through a Session
transaction. Rebinding a mesh with a non-rest pose is rejected until the
user resets or explicitly removes that pose; no authored state is silently
discarded. A Part cannot detach while it owns bindings. Deleting a bound
substitution requires explicit unbinding first. Duplication remaps copied
drawing IDs, while a linked-artwork branch keeps the referenced drawing ID.

Validation rejects missing/wrong-Part drawing references, changed source
dimensions, duplicate bindings, excessive grids, nonfinite coordinates,
out-of-range UVs, out-of-proxy vectors and degenerate or flipped rest/pose
triangles. The image profile uses straight-alpha sRGB RGBA8. Regular grids
start at the source alpha bounds, preserving the original registered image at
rest while reducing posed proxy work. Vector-only drawings retain their
editable strokes; the graphics adapter rasterizes them at the scene's bounded
resolution before warping. Mixed image/vector and raster-tile substitutions
cannot bind in this profile. The document domain contains no QPainter, UI,
filesystem or library types.

The renderer uses a bounded CPU inverse-triangle UV map with premultiplied
bilinear sampling and one ownership rule for shared edges. It rejects folded
triangles and proxies over 4096 pixels on either axis. The existing Qt 6
QPainter adapter draws the result; the same result feeds Display and Write.
There is no new dependency. Eigen and tessellation remain registered
candidates for HM-06 solver/topology workloads; their upstream reputation is
not evidence of this mesh warp's performance. The owned warp is the fallback
if those libraries are not adopted. No GPU path or analytic vector deformation
is claimed.

Format-7 projects have no bindings on load. First save to format 8 creates a
source-version `.pre-v7.bak` copy before the SQLite migration transaction.
Older readers reject format 8. The bounded contract passed alpha/seam,
texture sampling, cancellation, native mouse, migration and renderer-cost
checks recorded in the acceptance document. The mesh pose is a saved static
preview, not an animated deformer. HM-06 owns stable animated bone and curve
properties, weight solving, control evaluation and their final quality budget.
