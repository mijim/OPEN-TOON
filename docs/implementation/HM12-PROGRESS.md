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
Format 24 adds **Bypass opacity** beside that control. The selected Drawing or
Part retains its setup opacity and keys while the typed node forwards its
image unchanged. A cutter uses the same forwarded source image; Display and
Write agree. The command undoes atomically and older projects default to
enabled opacity. See
[ADR-048](../architecture/adr/048-persistent-opacity-bypass.md).
Format 25 adds saved **Normal**, **Multiply** and **Screen** layer blend modes.
Nodes edits the selected Drawing or Part. The mode applies to the ordered
composite after its cutter and opacity. A painted cutter source keeps its own
mode; source alpha remains a fractional matte. Legacy and Linear sRGB use
their respective color spaces, while alpha stays source-over. See
[ADR-049](../architecture/adr/049-layer-blend-modes.md).
Format 28 adds **Add** to the same selector and typed graph. The overlap
clamps the sum of the two straight-color channels before the existing
premultiplied source-over calculation; fractional alpha and cutters keep
their behavior. Legacy and Linear sRGB apply the sum in their respective
spaces. See [ADR-054](../architecture/adr/054-additive-layer-blending.md).
Format 29 adds **Bypass blend** for a selected Drawing or Part. A typed
`Bypassed blend` node renders Normal source-over while retaining the saved
Multiply, Screen or Add choice. Re-enabling restores that choice without
reselecting it. See [ADR-055](../architecture/adr/055-persistent-blend-bypass.md).
Drawing cards can now be dragged onto another Drawing card to put the source
immediately above the target in the saved composite order. The drop target
highlights; the operation rejects missing, same and locked layers. The
document command is atomic, with undo/redo and save/reopen. This edits layer
order through the derived Nodes presentation; arbitrary node placement and
wiring remain open.
Alt-dragging a Drawing card onto another binds the first as the second's
cutter source through the existing validated document command. Its fractional
alpha, saved references and one-step undo use the same path as the inspector.
An ordinary drag continues to reorder. This is a direct graph connection for
the supported cutter recipe; arbitrary wiring remains open.
Alt-clicking an Opacity, Apply matte or non-Normal blend card toggles its
existing saved bypass through the same validated command as the toolbar.
Alt-clicking the bypassed card restores the retained setting. Locked layers
continue to reject the edit; the card label changes with graph evaluation.
The Nodes search field locates cards by layer name or node kind without
editing the document. It highlights matches, scrolls to the current result
and cycles through them with Enter or Next; arbitrary graph edits remain open.
An image node's preview can now be shown on the main canvas as an alternate
Display source. **Show final output** restores the ordinary scene. The
selection is view state, clears when its node identity changes, and never
alters the saved graph or Write/export terminal. See
[ADR-057](../architecture/adr/057-alternate-display-node.md).
Matte cards use the same canvas action but show source or inverted fractional
alpha as opaque grayscale. Intermediate Display images use the bounded
revision-aware cache with node ID in its key; see
[ADR-058](../architecture/adr/058-node-scoped-display-cache.md).
Cutter source, Invert matte, Apply matte and composite cards now carry their
owning Drawing or Part. Clicking one selects that exact source or target in
the inspector while preserving its image/matte preview. The metadata is
derived, validated and does not change saved pixels or project format. See
[ADR-056](../architecture/adr/056-derived-node-owner-selection.md).
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
- A keyed target and fractional cutter-source render test checks opacity
  bypass on either image and both together, including Display/Write,
  undo/redo and save/reopen. A separate legacy scene without a matte checks
  that bypass enters the graph and does not recursively render its source.
  A format-23 migration retains a readable
  `.pre-v23.bak`; native Qt Quick smoke clicks bypass, undoes, redoes and
  reopens it.
- Fractional overlapping image tests check Normal, Multiply and Screen in
  both color profiles, Display/Write, invalid mode rejection, atomic undo,
  save/reopen and format-24 migration backup. Native Qt Quick smoke selects
  Multiply from the actual popup on a painted cutter source and checks its
  pixel difference, undo and reopened output.
- The same fractional image fixture now checks Add in both color profiles,
  Display/Write parity, undo/redo and reopened pixels. A format-27 migration
  retains Screen, writes a readable `.pre-v27.bak` and rejects Add claimed by
  an older file. Native Qt Quick smoke clicks Add on a painted cutter source,
  checks brighter pixels, undoes and reopens the result.
- A three-opaque-color fixture checks exact order and rendered top color,
- Fractional overlap now checks blend bypass against Normal pixels, the typed
  bypass node, Write parity, one-step undo/redo and save/reopen. Format-28
  migration preserves Add with bypass off and a readable backup. Native Qt
  Quick smoke clicks bypass on an Add-blended painted source, then verifies
  retained Add, rendered pixels, undo/redo and reopen.
- A three-opaque-color fixture checks exact order and rendered top color,
  invalid/same/locked targets, one-step undo/redo and save/reopen. Native Qt
  Quick smoke drags a Drawing card onto a second card, observes the target
  highlight and checks the new output pixels before and after undo/reopen.
- Native Qt Quick smoke types Write into the search field in a narrower window,
- Native Qt Quick smoke Alt-drags a Drawing source onto a Drawing target,
  checks the target binding and output color, then undoes, redoes and reopens.
- Native Qt Quick smoke Alt-clicks Apply matte to bypass, undoes, redoes and
  Alt-clicks the resulting Bypassed cutter card to re-enable the same binding.
- Native Qt Quick smoke routes an isolated blue Drawing to the canvas while
  final Write stays the fractional red target, checks pixels and document
  revision, restores the final view and rejects a stale node after reorder.
- Native Qt Quick smoke samples 64/255 source and 191/255 inverse matte
  grayscale on the canvas, with unchanged final Write alpha. A cache test
  distinguishes two node outputs and invalidates after document revision.
- Graph tests check derived cutter, matte and composite owner IDs and reject
  dangling owners. Native Qt Quick smoke clicks the Cutter, Apply matte and
  Composite cards and checks that the intended layer becomes selected.
- Native Qt Quick smoke types Write into the search field in a narrower window,
  checks that the graph scrolls to the result with unchanged output, then
  finds three Drawing cards and clicks Next to visit the second result.
- A current three-frame original-art run with five Add layers measured
  50.93 ms/frame in Legacy and 45.22 ms/frame in Linear sRGB. The same host
  run measured Multiply/Screen at 45.79/45.18 ms and Normal at 1.84/42.99 ms.
  These are short-run means, not p95 interaction times.
- The original 20-layer 1080p fixture measured 49.36 ms/frame for five
  alternating Multiply/Screen layers in Legacy and 43.59 ms/frame in Linear
  sRGB, averaged over three frames on the M1 Pro. Normal measured 1.62 and
  43.32 ms/frame respectively; these are renderer costs, not input latency.
- Local macOS `build/locked`: full CTest and native HM-12 results are recorded
  in the current release entry.

## Open contract

The arbitrary editable node graph, general node bypass beyond opacity, broader transform
presentation, group ports, group/ungroup and full arm/eye overlap recipe are pending. HM-12
and its P10 owning phase remain open.
