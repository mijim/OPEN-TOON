# HM-02 registered artwork intake — accepted bounded contract

The original [Clockwork Hello](../../tests/fixtures/harmony-moment/README.md)
PNG parts and sampled vector baseline can be loaded, saved, reopened and rendered
without changing imported RGBA bytes, registration offsets or palette IDs. The
registered-parts command requires equal canvas dimensions and transparent PNG
input, interprets untagged assets as sRGB, rejects incompatible tagged profiles,
sorts names deterministically and creates one undoable layer set. The sequence
command sorts fixed-width frame numbers, leaves missing numbers empty, extends
duration when needed and reports gaps. A cancelled chooser invokes no command.
Corrupt, duplicate, mismatched, oversized or over-budget batches publish no
partial scene.

Single-image import additionally converts tagged input to sRGB. If no layer is
selected because the scene has no layers, it creates the layer and drawing in one
transaction. Its undo, save/reopen and byte-level color conversion are covered by
`Single image import creates its layer atomically and converts tagged color to
sRGB` in `tests/export_tests.cpp`. The same executable checks registered-part
pixels, gap semantics, order, color-ID preservation, undo/redo and failed batches.
`tests/harmony_reference_tests.cpp` checks the original rigid PNG scene,
exact first-frame output and format-3 reopen. The macOS 2026-09-22 locked build
passed all 68 CTest entries and the native UI smoke.

HM-02 is the **artwork-intake contract**, not completion of P02. The app still
lacks layered PSD/SVG import, editable image placement/registration guides, richer
brush and vector topology tools, physical-tablet validation, production-scale
performance qualification and the final animator journey. Part roles, pegs,
substitution sets and rig controls belong to later slices.
