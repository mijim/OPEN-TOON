# Functional catalog

283 original proposed capabilities across 26 domains. Application status: **experimental, with partial working subsets; no phase is complete**.

Canonical source: [features.json](features.json). Acceptance criteria are OPEN-TOON objectives. Domain dependencies express relationships, not a requirement to finish an entire domain before starting another.

Levels: `core` foundations; `pro` professional workflow; `advanced` high complexity; `optional` optional extension; `legacy` historical compatibility. These are not execution phases.

| Domain | Capabilities | Module |
|---|---:|---|
| [PRJ — Projects, scenes and persistence](domains/prj.md) | 10 | `project` |
| [UI — Workspace, navigation and preferences](domains/ui.md) | 10 | `workspace` |
| [LYR — Layers, drawings and art sublayers](domains/lyr.md) | 11 | `document` |
| [VEC — Vector drawing and geometry editing](domains/vec.md) | 27 | `drawing-vector` |
| [RAS — Bitmap drawing, textures and brushes](domains/ras.md) | 7 | `drawing-raster` |
| [COL — Painting, palettes and color management](domains/col.md) | 15 | `colour` |
| [TIM — Timeline, Xsheet and exposure](domains/tim.md) | 14 | `timeline` |
| [FBF — Traditional and paperless animation](domains/fbf.md) | 12 | `animation-drawing` |
| [ANI — Transforms, curves and keyframe animation](domains/ani.md) | 13 | `animation-curves` |
| [RIG — Rigging, hierarchies and inverse kinematics](domains/rig.md) | 15 | `rigging` |
| [DEF — Deformers and meshes](domains/def.md) | 15 | `deformation` |
| [CTL — Character controllers and dashboards](domains/ctl.md) | 8 | `controllers` |
| [MOR — Vector morphing](domains/mor.md) | 6 | `morphing` |
| [CAM — Camera, staging and 2.5D space](domains/cam.md) | 7 | `camera` |
| [NOD — Compositing graph](domains/nod.md) | 10 | `compositor-graph` |
| [FX — Effects, advanced compositing and shading](domains/fx.md) | 20 | `effects` |
| [PAR — Particles](domains/par.md) | 7 | `particles` |
| [AUD — Sound and lip sync](domains/aud.md) | 10 | `audio` |
| [IMP — Import, scanning and interchange](domains/imp.md) | 10 | `interchange` |
| [THR — 3D integration](domains/thr.md) | 9 | `integration-3d` |
| [OUT — Preview, rendering and export](domains/out.md) | 13 | `render` |
| [LIB — Libraries, symbols and reuse](domains/lib.md) | 7 | `asset-library` |
| [GAM — Game output](domains/gam.md) | 7 | `game-export` |
| [AUT — Scripting and automation](domains/aut.md) | 6 | `scripting` |
| [STU — Studio production and collaboration](domains/stu.md) | 8 | `studio` |
| [AIX — Optional AI assistance](domains/aix.md) | 6 | `ai-adapters` |

## Related catalogs

- [Capability JSON schema](features.schema.json).
- [Nonfunctional requirements](nonfunctional.json).
- [Node inventory by category](nodes.md).
- [Integrated acceptance workflows](../research/02-workflows.md).

Do not add capability, page and node counts: they overlap and have different levels of detail.
