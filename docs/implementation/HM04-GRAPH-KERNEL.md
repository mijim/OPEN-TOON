# HM-04 typed composition kernel — working subset

This bounded implementation adds ten observable capabilities without claiming
the HM-04 contract or P10 phase complete:

1. Compile ordered Drawing and Part layers into a derived composition DAG.
2. Check typed Image, Transform and Matte input/output contracts.
3. Reject cycles, missing nodes/layers, duplicate IDs/slots and invalid outputs.
4. Evaluate nodes in dependency order and visit only the requested output branch.
5. Render a layer source using its exposure, hierarchy transform and visibility.
6. Read an evaluated local transform through a separate Transform node.
7. Composite image layers with a bounded premultiplied linear-sRGB `Over` path.
8. Extract an alpha Matte from an image and apply it to another image.
9. Route distinct Display and Write outputs through the shared evaluator.
10. Save and undo an opt-in composition profile while keeping old-format scenes
    on their original direct-painter appearance; format-5 migration backs up
    the original project before the successful format-6 save.

The graph also reports descendants affected by a layer change. This is a
dependency query, not yet a revision-safe render cache or publication protocol.
The graph is derived at runtime; users can select the composition profile in
**View → Composition**, but cannot yet edit node topology or connect masks in
the UI. Render tests cover graph rejection, alpha/matte behavior, output
equivalence, scene reopening and canvas painting. The remaining work after
that first block is addressed below and in
[ADR-027](../architecture/adr/027-typed-composition-profile.md).

## Second bounded block — revision-safe preview and throughput

Ten additional behaviors now work on the macOS development build:

1. Repainting the unchanged linear canvas reuses its composited image.
2. Frame, background, onion-skin, output and profile options have distinct cache keys.
3. A new document generation or revision invalidates older cached frames.
4. The cache evicts least-recently-used frames within a 96 MiB preview budget.
5. Render tickets reject late publication after a newer request, revision or clear.
6. Linear rendering checks cancellation during graph evaluation and image rows.
7. PNG export treats a mid-frame cancellation as a partial, cancelled job.
8. Graph ordering and affected-node traversal handle deep chains iteratively.
9. Changing a parent peg identifies the descendant Part image and its outputs.
10. Linear compositing uses bounded ink regions, fast transparent/opaque paths
    and color lookup tables while preserving tested alpha coverage.

Export runs on an immutable worker snapshot. The next bounded block adds a
background preview producer to the actual canvas. The 19-layer synthetic workload
and measured limits are in
[the compositor benchmark](COMPOSITOR-BENCHMARK.md).

## Third bounded block — speculative canvas preview and original artwork

Ten additional observable behaviors now work on the macOS development build:

1. A linear canvas repaint queues the next scene frame, including the loop boundary.
2. A missed current frame still renders immediately, preserving accurate scrubbing.
3. The queued frame evaluates an immutable document snapshot on a worker thread.
4. Only one producer runs; a newer request replaces its one pending slot.
5. Repeated requests for a pending, running or cached frame do not start duplicate work.
6. Scene edits, view-option changes and canvas replacement cancel speculative work.
7. Render-row cancellation and revision tickets prevent obsolete results from publishing.
8. Frames too large for the preview cache are not queued speculatively.
9. Original 19-part sRGB artwork retains exact alpha coverage between the legacy and
   linear profiles at the tested half-resolution pose, including transparent pixels.
10. That original-art scene produces identical Display, Write and reopened-project
    pixels at three tested poses; the 1080p workload has a recorded cost baseline.

This is speculative preparation, not a separate evaluator or a promise of real-time
playback on all scenes. The native smoke covers the foreground canvas cache/profile
path; the producer's immutable snapshot and publication behavior have an adapter
test. A timed native playback/scrub journey, fuller color charts and denser scene
cost qualification remain before HM-04 contract acceptance. The graph still has no
editable node topology in the UI.
