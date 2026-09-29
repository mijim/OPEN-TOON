# HM-12 composition progress

HM-12 is in progress. Format 18 saves a Cutter matte source ID on a Drawing
or Part layer. The compact inspector lists valid visible Drawing and Part
sources and offers `No cutter matte` to bypass. An assignment is one undoable
document command. The source's fractional alpha clips the target, and the
source does not paint into the final composite. Rendering uses the existing
typed graph for both display and write output, regardless of color profile.
The target retains its layer order. A source can mask more than one target;
missing, self, hidden and chained sources are rejected. Removing a referenced
source fails before modifying the document. Independent character copies
remap bindings inside the copied branch; an external source blocks branch
copy. See [ADR-039](../architecture/adr/039-layer-cutter-matte.md).

## Verification

- `tests/composition_graph_tests.cpp` checks the generated matte nodes and
  rejects missing, self, hidden and chained references.
- `tests/render_tests.cpp` checks 128/255 target alpha multiplied by 128/255
  source alpha to 64/255 output, source exclusion, display/write agreement,
  format-18 serialization and the unbound composite.
- `--hm12-smoke` opens a saved fixture in native Qt Quick, finds the inspector
  control, binds a matte, checks rendered pixels, saves and reopens, then
  bypasses and undoes the change. The screenshot is a local test artifact.

## Open contract

The arbitrary editable node graph, Opacity and transform presentation,
group ports, group/ungroup and full arm/eye overlap recipe are pending. HM-12
and its P10 owning phase remain open.
