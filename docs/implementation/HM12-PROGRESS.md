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
copy. The resizable bottom workspace now has a Nodes tab showing the derived
typed Drawing, Cutter, Apply matte, Composite, Display and Write nodes with
numbered input references. Clicking a Drawing node selects its layer; the
same panel can change composite order and assign or bypass its cutter. This
is a focused presentation of document-owned operations, not arbitrary graph
persistence. Format 19 adds an Outside choice, which inverts the source's
fractional alpha across the full scene through a typed `InvertMatte` node.
Removing the binding also clears inversion. See
[ADR-039](../architecture/adr/039-layer-cutter-matte.md) and
[ADR-040](../architecture/adr/040-inverted-cutter-matte.md).

## Verification

- `tests/composition_graph_tests.cpp` checks the generated matte nodes and
  rejects missing, self, hidden and chained references.
- `tests/render_tests.cpp` checks 128/255 target alpha multiplied by 128/255
  source alpha to 64/255 output, source exclusion, display/write agreement,
  format-18 serialization and the unbound composite. It also checks
  128/255 target against 64/255 source: Inside yields 32/255 and Outside
  yields 96/255 in both outputs and after reopen. An inverted cutter keeps
  target ink outside the source's ink bounds.
- `--hm12-smoke` opens a saved fixture in native Qt Quick, finds the inspector
  and Nodes panel, binds a matte, checks rendered pixels, saves and reopens,
  then bypasses and undoes the change. The screenshot is a local test artifact.
- `tests/export_tests.cpp` checks that the presented graph changes with an
  assignment, inversion and undo, including both output terminals. Rigging tests reject
  referenced-source deletion and external-source branch copies atomically;
  full character copies remap the source.
- Storage tests load a format-18 matte with Inside as the default and keep a
  readable `.pre-v18.bak` after the first format-19 save.

## Open contract

The arbitrary editable node graph, Opacity and transform presentation,
group ports, group/ungroup and full arm/eye overlap recipe are pending. HM-12
and its P10 owning phase remain open.
