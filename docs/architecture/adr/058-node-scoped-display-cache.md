# ADR-058 — Node-scoped Display cache

Status: experimental HM-12 subset, 2026-09-29.

## Decision

The revision-aware image cache key now includes a compositor node ID. Zero
means the existing final Display or Write terminal; nonzero distinguishes an
intermediate image or matte displayed on the canvas. The key still includes
scene generation, document revision, frame, size, composition profile and view
options. Changing a node, frame or revision cannot reuse pixels from a
different output. The existing bounded LRU budget and stale-publication
checks apply unchanged.

MatteFromImage and InvertMatte Display images convert fractional alpha to
opaque grayscale after graph evaluation and before entering this cache. The
conversion affects only the diagnostic Display image. Write and PNG export
still consume the original matte alpha and final composite.

## Evidence and limits

A cache test switches red and blue node IDs at the same revision, confirms
reuse when returning to the first ID, then checks a new revision renders
again. Native Qt Quick smoke samples the 64/255 source matte and 191/255
inverse matte on the canvas while Write retains its 32/255 and 96/255 target
alpha. The locked macOS suite passes 187/187 CTest entries.

The bounded cache does not imply a measured p95 latency for large interactive
graphs. The derived graph is still rebuilt when its identity is checked;
arbitrary persisted graph layout and group ports remain open.
