# Milo

A three-quarter cartoon character drawn as editable SVG artwork for OPEN-TOON.
Blond hair, a long face, a blue shirt and terracotta trousers follow the visual
direction of the user-provided rig reference. The paths were drawn for this task;
no raster reference, tracing embed, font or external resource is included.
Original contributions are licensed GPL-3.0-or-later under the repository license.

## Files

- `milo.svg`: editable vector master, transparent 800 × 1080 canvas, 21 named groups in back-to-front order. Solid fills and round strokes; no filters, masks, linked images or fonts.
- `milo.png`: transparent assembled character.
- `preview.png`: white-background visual preview; do not import this as a character part.
- `png-parts/`: 21 transparent PNG pieces, all using the exact same 800 × 1080 canvas.
- `parts.json`: suggested parent relationships and pivots in canvas pixels.
- `milo-import-kit.zip`: optional packaged master, previews and registered
  parts created by `export.py`; the source tree keeps the editable files.
- `continuous-parts/`: 17 registered PNGs for the editable Milo study. Each arm
  and leg is one image; the middle elbow or knee is a bone joint inside it.
- `continuous-rig.json`: source grouping, rest joints, follower Parts and the
  sample pose. `generate_continuous_parts.py` rebuilds the 17 PNGs from the
  original 21 exports without cropping their common canvas.

## Use in OPEN-TOON

The current documented import contract supports registered PNG parts, not layered SVG.
Keep the SVG as the editable source. In OPEN-TOON, choose **File → Import registered
PNG parts…** and select all 21 images inside `png-parts/`. Keep the numeric filename
prefixes: ascending names match the artwork's back-to-front order. Do not crop the
parts independently. The importer centers the common canvas in the scene.

For example, in a 1920 × 1080 scene the artwork canvas origin is at (560, 0).
A pivot listed as (409, 624) therefore starts at scene position (969, 624), before
any subsequent transforms. Use `parts.json` as a manual guide when assembling
Parts/Pegs and setting pivots in Setup mode. It is not an auto-import rig format.

The character is resting cut-out artwork, not a finished deformation rig. Head
and neck share one part; eyes/brows/nose share another. The shirt includes both
sleeves. Large shoulder, knee or neck rotations may need adjusted overlaps,
separate sleeves or replacement drawings. Hands and mouth have one pose each.
No skeleton controls are painted into the artwork.

For actual deformation, open [`examples/milo-continuous.otoon`](../../../examples/milo-continuous.otoon).
It contains the 17-Part character, four alpha-following contour meshes,
two-segment bones in each complete limb, linked hands and ankles, and a
48-frame bend-and-return test. Frame 24 shows the bend; frame 47 returns to
rest. The SVG and 21-part import kit remain the editable source. The example
uses the existing saved project format; no new runtime dependency is required.

## Verification and regeneration

The master was rasterized and visually reviewed on macOS. Each PNG is RGBA with
transparent margins and identical dimensions. Compositing the exported pieces
in filename order is checked against the master on white, allowing only tiny
antialiasing-rounding differences. The character remains within the canvas.
This asset check does not claim an interactive import/save/reopen test in the app.
The separate continuous example is checked by the renderer and native Qt Quick
application through frame changes, save and reopen. See
[`HM06-MILO.md`](../../../docs/implementation/HM06-MILO.md) for evidence and
remaining artistic limits.

To regenerate, use Python with optional asset-only tools `cairosvg` and `Pillow`,
then run `python export.py` from this directory. CairoSVG also requires Cairo;
on Homebrew macOS its library directory may need to be supplied through
`DYLD_FALLBACK_LIBRARY_PATH`. These tools are not application dependencies.
