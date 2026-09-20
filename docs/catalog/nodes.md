# Inventario de nodos

193 entradas de operadores y familias propuestas. No se presentan como ese número de efectos distintos. Todas están pendientes de especificación y parámetros de OPEN-TOON.

Antes de implementar cada operador: definir puertos, tipos, parámetros/unidades, valores por defecto, animabilidad, espacio de color, alfa, bounds/halo de tiles, ROI, invalidación, determinismo, errores, perfiles soportados y una escena de referencia.

El núcleo mínimo de composición y las principales familias ya están descritos en NOD, FX, PAR, DEF, CTL y THR. Este inventario evita perder los operadores menos frecuentes durante el plan largo.

## 3d

| ID estable | Nodo |
|---|---|
| `NODE-450dce4d56` | 3D |
| `NODE-e0088fd808` | 3D Kinematic Output |
| `NODE-59d78de94b` | Flatten |
| `NODE-27398d62d4` | Render Maya Batch |
| `NODE-13bbeb4faa` | Render Blender Batch |
| `NODE-09dc0cbf72` | Subnode Override Node |
| `NODE-4bb4003c85` | Subnode Animation |
| `NODE-2e596c092a` | Toon Shader |
| `NODE-b02bff22a8` | Z Buffer Smoothing |

## combine

| ID estable | Nodo |
|---|---|
| `NODE-303ad8d494` | Combine |
| `NODE-f06f7a2f68` | Blending |
| `NODE-5abeeb75d8` | Composite |
| `NODE-4ed89889be` | Composite-Generic |
| `NODE-3262d0b15b` | Cutter |
| `NODE-85b5cbb854` | Highlight |
| `NODE-0b90fa5495` | Image Switch |
| `NODE-b62d5c20ab` | Matte-Composite |
| `NODE-1354148e3f` | Tone |
| `NODE-a439ffc011` | Transformation Switch |

## constraint

| ID estable | Nodo |
|---|---|
| `NODE-f3a568dc7a` | Constraint |
| `NODE-6eff2bf33f` | Constraint Switch |
| `NODE-6cfa5e7153` | Dynamic Spring Node |
| `NODE-c7475645cf` | Multi-Points Constraint |
| `NODE-dc168aeb80` | Static Transformation |
| `NODE-a8e7081e39` | Three-Points Constraint |
| `NODE-c3bac94d45` | Transformation Gate |
| `NODE-1c89563d25` | Transformation Limit |
| `NODE-7fb433fb89` | Two-Points Constraint |

## controller

| ID estable | Nodo |
|---|---|
| `NODE-968d914aee` | Controller |
| `NODE-aae033e5e0` | OGL-Controller |

## deformation

| ID estable | Nodo |
|---|---|
| `NODE-1701509f0b` | Deformation |
| `NODE-e451ae614f` | Articulation |
| `NODE-fca74eed55` | Auto Fold Node |
| `NODE-9e183f1471` | Auto Muscle |
| `NODE-81d8ec415e` | Bone |
| `NODE-dd50b78b1a` | Curve |
| `NODE-b9491e3784` | Deformation Composite |
| `NODE-5973dd3d5a` | Deformation Root |
| `NODE-a3c5b76f87` | Deformation Scale |
| `NODE-9b0a4d9526` | Deformation Switch |
| `NODE-f454fa7582` | Deformation Uniform Scale |
| `NODE-1d5addd136` | Deformation Wave |
| `NODE-54856eaa8c` | Fold |
| `NODE-9db97dfc3c` | Game Bone |
| `NODE-c37ba7421a` | Glue |
| `NODE-0ac75ee0c4` | Kinematic Output |
| `NODE-2ef7def4eb` | Mesh Warp |
| `NODE-3d446d1153` | Offset |
| `NODE-b049fd6684` | Point Kinematic Output |
| `NODE-d85270ef67` | Refract |
| `NODE-1ae3c2d6f0` | Shape-Aware Deformation |
| `NODE-6239a7d604` | Stick |
| `NODE-1a92cef5c5` | Turbulence |
| `NODE-79042603fb` | Weighted Curve Node |
| `NODE-a1b75fc72e` | Weighted Deform |
| `NODE-404d536272` | Weighted Line Node |
| `NODE-6f6d20f761` | Weighted Point Node |
| `NODE-08f3cc6941` | Weighted-Drawing Node |

## filter

| ID estable | Nodo |
|---|---|
| `NODE-34cc3c5ef1` | Filter |
| `NODE-a94126b0bc` | Anti-Flicker |
| `NODE-fea4bf1696` | Animate Pencil Line Texture |
| `NODE-be5265b356` | Auto-Patch |
| `NODE-54340ff712` | Bloom |
| `NODE-cd9bdd6a5c` | Blur-Box |
| `NODE-28e523f01d` | Blur-Directional |
| `NODE-674d718b26` | Blur-Gaussian |
| `NODE-a002538963` | Blur-Radial |
| `NODE-83b0cb533a` | Blur-Radial-Zoom |
| `NODE-befac9a195` | Blur-Variable |
| `NODE-e2dd78046b` | Bokeh-Blur |
| `NODE-0a2a946319` | Channel Selector |
| `NODE-5b3f9fff47` | Channel Swap |
| `NODE-13adf1b127` | Chroma-Keying |
| `NODE-284bfc4093` | Colour Banding |
| `NODE-d30174d7ca` | Colour-Art |
| `NODE-efa4ccddee` | Colour Curves |
| `NODE-481cf50ed6` | Colour-Fade |
| `NODE-3e9aa894ef` | Colour-Levels |
| `NODE-345b712f48` | Colour-Override |
| `NODE-64b8853d40` | Colour-Scale |
| `NODE-14c7d66ca4` | Colour-Selector |
| `NODE-0235c2970b` | Contrast |
| `NODE-26bc0eede4` | Dither |
| `NODE-136b52d28c` | Focus |
| `NODE-6ac02956c2` | Focus Multiplier |
| `NODE-609aeaf262` | Gamma |
| `NODE-0f28d1dc9a` | Glow |
| `NODE-3627cbb7f3` | Grain |
| `NODE-9225de77b3` | Greyscale |
| `NODE-a534097217` | Hue-Saturation |
| `NODE-9005c38a21` | Increase-Opacity |
| `NODE-d9af013dca` | Layer Selector |
| `NODE-a348da5ab4` | Line Art |
| `NODE-569964f17a` | Luminance Threshold |
| `NODE-909074b8b0` | Matte-Blur |
| `NODE-edbfbf19fc` | Matte-Resize |
| `NODE-912f5e2635` | Median |
| `NODE-f9b99d66da` | Motion-Blur |
| `NODE-946b242696` | Motion-Blur (Legacy) |
| `NODE-7abc084bfd` | Negate |
| `NODE-05201cb529` | Overlay Layer |
| `NODE-5ecef64ef3` | Pixelate |
| `NODE-df907b5949` | Remove Transparency |
| `NODE-8027641246` | RGB-Difference-Keyer |
| `NODE-92aba889fe` | Shadow |
| `NODE-26c2455619` | Shine Node |
| `NODE-b69e81e91d` | Transparency |
| `NODE-1605954b8a` | Underlay Layer |
| `NODE-83704e7075` | Unsharp Mask Node |

## general

| ID estable | Nodo |
|---|---|
| `NODE-5f0dde20ab` | Layer Properties |
| `NODE-a06b79d317` | Nodes / Layers |

## generator

| ID estable | Nodo |
|---|---|
| `NODE-9b7bf3ee52` | Generator |
| `NODE-62d4df6fb1` | Animated Matte Generator |
| `NODE-4c68960599` | Burn-In |
| `NODE-27da81224e` | Colour Card |
| `NODE-ce63ae5797` | Element / Drawing |
| `NODE-7a8b1eb580` | Field Chart |
| `NODE-4f0de97918` | Gradient |
| `NODE-166991ba4f` | Grid |
| `NODE-c04c6d8bb7` | Lens Flare |
| `NODE-ece7678ae8` | Turbulent Noise |

## group

| ID estable | Nodo |
|---|---|
| `NODE-509ac67271` | Group |
| `NODE-2c01d639e3` | Group |
| `NODE-60b8b6a9a5` | Multi-Port-In |
| `NODE-aaa0854231` | Multi-Port-Out |

## miscellaneous

| ID estable | Nodo |
|---|---|
| `NODE-c04d252ac9` | Miscellaneous |
| `NODE-5279f7012c` | External |
| `NODE-d7197c2eb3` | Master Controller |
| `NODE-fa38c883e3` | Note |
| `NODE-45b5a5aba5` | OpenGL Bypass |
| `NODE-890f4b7c1c` | OpenGL-Cache-Lock |
| `NODE-2cbd28e138` | Pre-Render Cache |
| `NODE-487343e338` | Render Preview |
| `NODE-1849afecb3` | Script |
| `NODE-3caeb3fbee` | Visibility |

## move

| ID estable | Nodo |
|---|---|
| `NODE-c3ba0f4da2` | Move |
| `NODE-35c19f293b` | Apply Image Transformation |
| `NODE-8009750100` | Apply Peg Transformation |
| `NODE-2d71e5f74e` | Camera |
| `NODE-a9c2c1b6bd` | OrthoLock |
| `NODE-50825d1409` | Peg |
| `NODE-dc6f0714ce` | Shake |
| `NODE-84f2d26aed` | Transform-Loop |
| `NODE-ea2b9f7d7b` | Quadmap |
| `NODE-3378009639` | Quake |

## output

| ID estable | Nodo |
|---|---|
| `NODE-fa7e646fa7` | Output |
| `NODE-fefc1d44a6` | Crop |
| `NODE-3840e4e148` | Display |
| `NODE-82fe430e0d` | Multi-Layer Write |
| `NODE-7c79115cdd` | Scale Output |
| `NODE-798d7924ab` | Write |

## particle-effects

| ID estable | Nodo |
|---|---|
| `NODE-566a6f74a9` | Particle |
| `NODE-bdfad01957` | 3D Region |
| `NODE-6a6d9b674d` | Baker |
| `NODE-fafb647dee` | Baker Composite |
| `NODE-bc447263a6` | Bounce |
| `NODE-d11c3edc75` | Explosion |
| `NODE-ee964793a7` | Gravity |
| `NODE-c16d8060bc` | Image Fracture |
| `NODE-59e2568aa3` | Kill |
| `NODE-04d023fb8a` | Move Particles |
| `NODE-dc58037115` | Orbit |
| `NODE-e2dae65f68` | Planar Region |
| `NODE-6a22dd859d` | Random |
| `NODE-8f72a14c36` | Region Composite |
| `NODE-946696dcd3` | Repulse |
| `NODE-a231fa7d28` | Rotation Velocity |
| `NODE-55c6e9c1c6` | Sink |
| `NODE-d1d0dd140d` | Size |
| `NODE-42286bad6a` | Sprite Emitter |
| `NODE-241434a0c5` | System Composite |
| `NODE-04b955d55b` | Velocity |
| `NODE-64b44b1ea1` | Visualizer |
| `NODE-dee6d2bda1` | Vortex |
| `NODE-6dea93eb54` | Wind-Friction |

## plugins

| ID estable | Nodo |
|---|---|
| `NODE-494ccf4929` | Brightness Contrast |
| `NODE-e29b493a5e` | Plugins |
| `NODE-f609d4e5c0` | Sparkle |

## presets

| ID estable | Nodo |
|---|---|
| `NODE-8b6f458a81` | Selection Preset |

## shading

| ID estable | Nodo |
|---|---|
| `NODE-6e4f3b2399` | Shading |
| `NODE-95ad409179` | Ambient Occlusion |
| `NODE-f1a32d6965` | Cast Shadow |
| `NODE-9903c775ca` | Light Position |
| `NODE-2c62f10be1` | Light Shader |
| `NODE-747a8cd407` | Normal Map |
| `NODE-9061822295` | Normal Map Converter |
| `NODE-6b0896ff25` | Shape Curve |
| `NODE-952bd13ba5` | Shape Line |
| `NODE-7409226e67` | Shape Render |
| `NODE-290455862d` | Surface Map |
| `NODE-00cdc0d7ac` | Surface Normal |
| `NODE-291449ca85` | Tone Shader |
| `NODE-0280e513cc` | Volume Object |
