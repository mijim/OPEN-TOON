# HM-05 bounded rest-binding and render acceptance

The HM-05 engineering contract is accepted for the tested macOS 15.5 arm64
profile. This establishes a saved rest/UV representation and an actual
texture-warp path for HM-06. It does not complete P09 or any full DEF catalog
capability. No animated bone, curve, influence radius or deformer key track is
present yet.

## Delivered behavior

- Format 8 saves a mesh per Part substitution with separate rest, static pose
  preview and normalized UV coordinates. A regular 2×2 or 4×4 grid can be
  bound from the selected drawing in Properties. Mouse drags edit vertices
  directly on the canvas; Rest and Pose modes, reset and removal are undoable.
- Image meshes start at the nontransparent source bounds. At rest, registered
  PNG pixels remain identical; a posed mesh uses a bounded premultiplied
  bilinear warp. Vector-only drawings keep their editable source strokes and
  use a scene-resolution raster proxy. Mixed image/vector or raster-tile
  drawings need a later profile.
- The binding belongs to the substitution identity. Switching drawings selects
  their own binding; independent copies remap it and linked artwork can keep
  a separate Part binding. Rebinding an edited pose reports that the pose
  would be discarded and rejects the command. Removing a bound substitution
  or detaching a bound Part is rejected until its binding is removed.
- The render proxy is limited to 4096 pixels per axis. Nonfinite, folded,
  degenerate, out-of-range and incompatible source data reject before commit.
  Cancelled renders do not publish a partial frame.

## Verification

- `tests/deformation_tests.cpp` checks identity, source compatibility, atomic
  rejection, undo/redo, duplicate/linked branches and format-8 round trips.
- `tests/mesh_render_tests.cpp` checks exact image rest pixels, edited rest
  and pose/reset stability, seam coverage, premultiplied transparent sampling,
  vector source editing, per-substitution switching, Display/Write equality,
  cancellation, reopened pixels and the original 19-part Harmony fixture.
- `tests/storage_tests.cpp` verifies format-7 to format-8 migration with a
  source-version `.pre-v7.bak` copy. The native `--smoke-test` drags Pose and
  Rest vertices with the mouse, previews the drag, undoes/redoes, rejects an
  unsafe rebind, resets, saves/reopens and removes the binding. The inspected
  [capture](hm05-mesh-ui.png) shows the checker distortion and visible handles.
- The macOS Release/RelWithDebInfo build passed all 114 CTest entries and the
  native smoke journey. `python3 scripts/validate_docs.py` passed after the
  status/roadmap update.

## Measured route and limits

On an Apple M1 Pro, macOS 15.5, AppleClang 17 and Qt 6.11.2, the CPU inverse
triangle sampler took 5.48 ms per 512×512 opaque 8×8 mesh warp. The complete
scene renderer took 4.79 ms per 1920×1080 frame with all 19 original Harmony
PNG parts posed on 2×2 meshes. Both figures are three-run local measurements
from an optimized build, not a general frame-rate guarantee. The latter fixture
uses the original 256×256 registered transparent parts; their alpha-bounded
meshes reduce processed pixels. Vector proxies and larger/full-opacity meshes
need their own workload qualification before production claims.

The 2026-09-28 toon-art revision retained the same 19 registered parts and
exact rest-pixel checks. Four optimized scene-renderer runs of its 19 posed
parts took 6.56–6.88 ms/frame (mean 6.69 ms) at 1920×1080 on the same host.
The original 4.79 ms measurement above remains historical for the prior art.
After the owner supplied a clearer hand-drawn rig direction, the independently
redrawn hoodie character measured 5.69–6.21 ms/frame (mean 5.91 ms) in four
further optimized runs. Rest-pixel and 19-part render checks still pass.

The accepted path uses the existing Qt 6 QPainter adapter plus small owned CPU
triangle/UV math. No additional dependency or license obligation was added.
Eigen and tessellation candidates stay unadopted until HM-06 has a measured
solver/topology workload and pinned revision/license evidence. HM-06 must
still deliver animated bone/curve controls, weights, extreme-bend quality and
its shot-level latency budget. Separate artist review of the complete shot
remains an HM-15 gate.
