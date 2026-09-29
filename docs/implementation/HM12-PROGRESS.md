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
The derived graph now inserts a typed Opacity node when a Drawing or Part
has non-unit setup or keyed opacity. The Nodes toolbar edits that existing
property. The cutter reads source alpha after opacity; target opacity is
applied before its cutter. This preserves fractional alpha, shared Display
and Write output, undo and saved transform keys without a new format field.
See [ADR-043](../architecture/adr/043-typed-layer-opacity-node.md).
Format 20 adds a persistent **Bypass cutter** control in Properties and Nodes.
Bypass keeps its source and Inside/Outside choice but shows the uncut target;
the source remains reserved and does not paint. Re-enable restores the exact
coverage. A derived `Bypassed cutter` node excludes the inactive source from
the output dependency chain. Removing the binding clears bypass. Formats
1–19 load with bypass off, and their first format-20 save retains a readable
source-version backup. See [ADR-044](../architecture/adr/044-persistent-cutter-bypass.md).
Format 21 adds **Paint cutter source** in Properties and Nodes. A cutter can
now stay in the ordered composite while also masking one or more targets.
The saved source flag defaults off for older projects. Source opacity feeds
both its painted image and matte; target bypass leaves the painted source
visible. The source and target must be unlocked to toggle it. See
[ADR-045](../architecture/adr/045-visible-cutter-source.md).
Clicking any image or matte card now opens a non-mutating node preview beside
the strip. The renderer evaluates only that node's dependency closure at a
bounded size; matte alpha is shown as grayscale. The preview follows frame
and document changes and clears if its node identity disappears. Drawing
cards still select their source layer. Transform-only nodes have no image
preview. This is diagnostic navigation, not a persisted graph edit.

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
- A one-pixel render test checks two Opacity nodes on a cutter and its
  target: 128/255 target alpha and 64/255 source alpha, each at 50% opacity,
  produce 8/255 Inside alpha. A keyed target at full opacity yields 16/255;
  Outside yields 56/255. Display/Write, native edit/undo and save/reopen agree.
- `--open PROJECT --hm12-smoke` loads the real continuous-character project
  before the fixture smoke. A missing startup project exits with an error.
- The cutter render test compares 32/255 Inside alpha with 128/255 bypassed
  target alpha and verifies that the blue source remains absent. Graph tests
  verify that a bypassed source cannot invalidate Write, and reject bypass
  without a source. A format-19 project migrates to bypass off; save/reopen
  retains a chosen bypass and `.pre-v19.bak` remains readable. Native Qt Quick
  smoke checks bypass, undo/redo, save/reopen and re-enable.
- Fractional-alpha render and graph tests check visible cutter pixels,
  Display/Write parity and a painted source that still reaches Write when its
  target is bypassed. Format-20 migration preserves a readable `.pre-v20.bak`;
  native smoke checks the visible control, undo/redo, save/reopen and disabling.
- A renderer test samples Drawing, Cutter and Apply matte node outputs and
  rejects an unknown node. Native Qt Quick smoke clicks the Drawing and Cutter
  cards, checks source and grayscale matte PNG pixels, and confirms the Image
  actually loads in the panel.
- Local macOS `build/locked`: 167/167 CTest entries and native HM-12 smoke pass.

## Open contract

The arbitrary editable node graph, general node bypass, broader transform
presentation, group ports, group/ungroup and full arm/eye overlap recipe are pending. HM-12
and its P10 owning phase remain open.
