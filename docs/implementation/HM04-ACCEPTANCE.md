# HM-04 bounded graph-kernel acceptance

The bounded HM-04 contract is accepted on the macOS development profile. It is
not completion of P10, a production renderer qualification, or an editable node
compositor. The accepted output profile is ordered Drawing/Part images, one
background, `Over`, alpha matte and explicit Display/Write terminals at RGBA8.
The saved `LegacyQt` profile preserves old-scene appearance; opt-in `LinearSrgb`
uses premultiplied linear-light composition.

Evidence:

- `tests/composition_graph_tests.cpp` rejects typed-port mismatches, cycles,
  dangling references and duplicate slots; parent changes identify descendants.
- `tests/render_tests.cpp` compares the alpha/color coverage chart and checks
  cancellation and stale-ticket rejection after newer requests and scenes.
- `tests/harmony_reference_tests.cpp` renders original 19-part sRGB artwork at
  three animated root poses; Display, Write, reopened output and headless pixels
  agree, with exact legacy/linear alpha at the tested half-resolution first pose.
- The native `--smoke-test` exercises background next-frame publication,
  current-frame preview, timed playback, rapid scrubbing, revision invalidation
  and undo on the actual QML canvas.
- `COMPOSITOR-BENCHMARK.md` records animated original-art costs and the preview
  memory budget. The 1080p linear reference is close to a 24 fps frame budget;
  it is a measured limit, not a promise of full-shot real-time playback.
- ADR-027 defines the solve order and keeps transform, camera and image graphs
  separate. The camera and deformation stages remain future consumers.

HM-13 may now implement one output camera. P10 still requires an editable node
workspace, useful effects, denser production workloads and later deformation/
camera integration. These obligations remain in the full phase roadmap.
