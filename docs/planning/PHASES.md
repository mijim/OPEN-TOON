# Phase index

> Generated from `roadmap.json`. Phase status is separate from feature completion; see [implementation evidence](../implementation/STATUS.md).

Implementation order is governed by the [Harmony Moment contract graph](HARMONY-MOMENT.md). Phase dependencies below concern full epic completion, not permission to start a bounded subset. Phase IDs are stable scope owners, not a serial schedule.

| Phase | Outcome / milestone | Depends on | Features | Engineer-weeks |
|---|---|---|---:|---:|
| [P00 — Technical feasibility and reuse](phases/P00.md) | Technical baseline | None | 0 | 8–16 |
| [P01 — Application foundation and English design system](phases/P01.md) | Foundation | None | 13 | 12–22 |
| [P02 — First complete frame-by-frame film](phases/P02.md) | v0.1 — usable vertical slice | P01 | 18 | 16–28 |
| [P03 — Timeline, Xsheet and paperless workflow](phases/P03.md) | v0.2 — paperless animation | P02 | 26 | 12–20 |
| [P04 — Professional vector drawing and palette workflow](phases/P04.md) | Vector production toolkit | P02 | 38 | 20–36 |
| [P05 — Raster and textured brush engine](phases/P05.md) | Mixed-media drawing | P02 | 8 | 14–26 |
| [P06 — Keyframe animation and multiplane cameras](phases/P06.md) | Keyframe animation | P01 | 19 | 16–28 |
| [P07 — Sound and practical media output](phases/P07.md) | v0.3 — drawing with sound | P01 | 15 | 14–26 |
| [P08 — Character assets, substitutions and accessible rig controls](phases/P08.md) | Reusable character authoring; Harmony Moment core subsets first | P02 | 24 | 23–42 |
| [P09 — Core deformation](phases/P09.md) | v0.6 — deformable characters | P01 | 12 | 22–40 |
| [P10 — Node compositor and managed color](phases/P10.md) | v0.8 — compositing | P01 | 25 | 20–38 |
| [P11 — Reliable 1.0 release](phases/P11.md) | v1.0 — reliable local 2D | P00, P01, P02, P03, P04, P05, P06, P07, P08, P09, P10 | 3 | 16–28 |
| [P12 — Automation and extension foundations](phases/P12.md) | Automation release | P11 | 8 | 14–28 |
| [P13 — Advanced constraints, pose grids and deformation controls](phases/P13.md) | Advanced character animation | P08, P09, P10 | 8 | 14–28 |
| [P14 — Morphing and drawing interpolation](phases/P14.md) | Morphing release | P04, P06 | 6 | 18–34 |
| [P15 — Advanced effects, particles and OpenFX](phases/P15.md) | Advanced compositing release | P10, P12 | 19 | 24–44 |
| [P16 — Production interchange](phases/P16.md) | Pipeline interoperability | P01 | 9 | 16–30 |
| [P17 — Mixed 2D and 3D scenes](phases/P17.md) | Mixed-media 3D release | P06, P10, P16 | 9 | 24–48 |
| [P18 — Game animation export](phases/P18.md) | Game delivery release | P08, P09, P16 | 7 | 16–30 |
| [P19 — Studio coordination and render queues](phases/P19.md) | Studio extension | P11, P12 | 8 | 24–48 |
| [P20 — Optional local AI tools](phases/P20.md) | Optional AI extension | P05 | 6 | 18–40 |
| [P21 — Legacy migration adapters](phases/P21.md) | Optional migration pack | P16, P18 | 2 | 8–18 |
| [P22 — Coverage audit and long-term maintenance](phases/P22.md) | Coverage audit and sustainable releases | P14, P17, P19, P20, P21 | 0 | 12–24 |

Total planning envelope: **381–722 focused engineer-weeks** including optional branches. See [estimation assumptions](ESTIMATES.md); this is not a calendar commitment.
