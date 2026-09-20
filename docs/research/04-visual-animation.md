# Visual animation and curve editing reference

Reviewed 2026-09-20. Product reference: Harmony Premium. This document describes
publicly documented behavior, not source-code reuse or a claim of implementation parity.

## Three different workflows

| Workflow | Harmony reference behavior | OPEN-TOON scope |
|---|---|---|
| Transform animation | Camera-view Transform gestures create or modify position/rotation/scale keys in an animation mode. | P06: direct layer pose gestures, full-pose keys and shared evaluation. |
| Drawing contour editing | Select/edit drawing points and handles to change the vector geometry. | P04: sampled point dragging with a live stroke preview; analytic contour handles remain open. |
| Morphing | A separate vector-only workflow interpolates source/destination drawings; hints establish correspondence between contours, pencil lines or regions. | P14: not implemented. Two transform keys do not imply drawing-shape interpolation. |

Sources: [Transform animation over frames](https://docs.toonboom.com/help/harmony-22/premium/motion-path/animate-several-keyframes.html),
[drawing and contour tools](https://docs.toonboom.com/help/harmony-25/premium/getting-started/drawing.html),
[morphing rules](https://docs.toonboom.com/help/harmony-25/premium/morphing/about-morph-rule.html),
[morph hints](https://docs.toonboom.com/help/harmony-24/premium/morphing/add-delete-morph-hint.html).

## Temporal curves and spatial paths

Harmony provides graphical function editors for parameter values and easing.
Its 3D-path workflow distinguishes spatial control points from timed keys and has
separate velocity controls. Consequently a parameter curve's tangent and a spatial
path control point must not be conflated.

Sources: [Functions](https://docs.toonboom.com/help/harmony-25/premium/motion-path/about-function.html),
[path controls](https://docs.toonboom.com/help/harmony-22/premium/motion-path/about-control.html),
[velocity curves](https://docs.toonboom.com/help/harmony-25/premium/motion-path/display-velocity-curve.html),
[function-view controls](https://docs.toonboom.com/help/harmony-25/premium/reference/menu/view/function-view-menu.html).

## Interaction decisions for this release

1. Create poses by moving artwork on the canvas with the Animate tool, at the chosen
   timeline frame. Preserve a starting pose when first animating later in the scene.
2. Keep the curve editor beside the timeline. Drag keys and round Bézier handles;
   retain numeric controls as a secondary precise/accessibility input.
3. Provide editable overshoot and let artists construct bounces with additional
   keys. Do not call a preset a physics simulation or drawing morph.
4. Keep drawing selection/point editing separate from layer pose animation, with
   explicit tool cursors and selected-target Properties.
5. Use a fixed screen-space picking margin so zooming out does not make thin vector
   strokes require pixel-perfect clicks.

The implementation contract and remaining limitations are in
[ADR-016](../architecture/adr/016-visual-animation-and-bezier.md).
