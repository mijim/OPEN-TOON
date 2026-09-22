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
equivalence, scene reopening and canvas painting. A real workload benchmark,
stale-job prevention and cross-domain solve ordering remain HM-04 acceptance
work. See [ADR-027](../architecture/adr/027-typed-composition-profile.md).
