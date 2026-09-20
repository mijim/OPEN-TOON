# Catálogo funcional

282 capacidades originales propuestas en 26 dominios. Estado de aplicación: **sin implementar**.

Fuente canónica: [features.json](features.json). Los criterios de aceptación son objetivos de OPEN-TOON; los enlaces documentan la familia de referencia. Las dependencias entre dominios indican relaciones, no que deba completarse un dominio entero antes de comenzar otro.

Niveles: `core` fundamentos; `pro` flujo profesional; `advanced` alta complejidad; `optional` optativo; `legacy` compatibilidad histórica. No son fases ni ediciones comerciales.

| Dominio | Capacidades | Módulo |
|---|---:|---|
| [PRJ — Proyectos, escenas y persistencia](domains/prj.md) | 10 | `project` |
| [UI — Espacio de trabajo, navegación y preferencias](domains/ui.md) | 10 | `workspace` |
| [LYR — Capas, dibujos y subcapas artísticas](domains/lyr.md) | 11 | `document` |
| [VEC — Dibujo vectorial y edición geométrica](domains/vec.md) | 27 | `drawing-vector` |
| [RAS — Dibujo bitmap, texturas y pinceles](domains/ras.md) | 7 | `drawing-raster` |
| [COL — Pintura, paletas y gestión de color](domains/col.md) | 15 | `colour` |
| [TIM — Timeline, Xsheet y exposición](domains/tim.md) | 14 | `timeline` |
| [FBF — Animación tradicional y paperless](domains/fbf.md) | 12 | `animation-drawing` |
| [ANI — Transformaciones, curvas y animación por claves](domains/ani.md) | 13 | `animation-curves` |
| [RIG — Rigging, jerarquías e inversa cinemática](domains/rig.md) | 14 | `rigging` |
| [DEF — Deformadores y mallas](domains/def.md) | 15 | `deformation` |
| [CTL — Controladores avanzados de personajes](domains/ctl.md) | 8 | `controllers` |
| [MOR — Morphing vectorial](domains/mor.md) | 6 | `morphing` |
| [CAM — Cámara, staging y espacio 2.5D](domains/cam.md) | 7 | `camera` |
| [NOD — Grafo de composición](domains/nod.md) | 10 | `compositor-graph` |
| [FX — Efectos, composición avanzada y sombreado](domains/fx.md) | 20 | `effects` |
| [PAR — Partículas](domains/par.md) | 7 | `particles` |
| [AUD — Sonido y sincronización labial](domains/aud.md) | 10 | `audio` |
| [IMP — Importación, escaneo e intercambio](domains/imp.md) | 10 | `interchange` |
| [THR — Integración 3D](domains/thr.md) | 9 | `integration-3d` |
| [OUT — Preview, render y exportación](domains/out.md) | 13 | `render` |
| [LIB — Bibliotecas, símbolos y reutilización](domains/lib.md) | 7 | `asset-library` |
| [GAM — Salida para videojuegos](domains/gam.md) | 7 | `game-export` |
| [AUT — Scripting y automatización](domains/aut.md) | 6 | `scripting` |
| [STU — Producción de estudio y colaboración](domains/stu.md) | 8 | `studio` |
| [AIX — Asistencia de IA opcional](domains/aix.md) | 6 | `ai-adapters` |

## Catálogos complementarios

- [Esquema JSON de las capacidades](features.schema.json).
- [Requisitos no funcionales](nonfunctional.json).
- [Referencias de nodos por categoría](nodes.md).
- [Inventario completo de navegación oficial](../research/reference-index.json).
- [Flujos integrados para aceptación](../research/02-workflows.md).

No sumar funciones, páginas y nodos: contienen solapamientos y niveles de granularidad distintos.
