# Fuentes y procedencia

Consulta: 2026-09-20. Fuentes primarias. Los enlaces se usan para documentar capacidades y decisiones; no se incorporan manuales ni recursos gráficos comerciales al repositorio.

## Referencia funcional

- [Harmony Premium 25: alcance y aplicaciones](https://docs.toonboom.com/help/harmony-25/premium/about/about.html): distinción entre editor, Database y aplicaciones auxiliares.
- [Guía de usuario Premium 25](https://docs.toonboom.com/help/harmony-25/premium/book/about-user-guide.html): punto de entrada del análisis funcional.
- [Índice de navegación oficial](https://docs.toonboom.com/help/harmony-25/premium/Data/Tocs/HAR__HAR_Online_Help.js): estructura utilizada para identificar áreas y URLs.
- [Datos del índice](https://docs.toonboom.com/help/harmony-25/premium/Data/Tocs/HAR__HAR_Online_Help_Chunk0.js): títulos de referencia y rutas.
- [Harmony 25](https://docs.toonboom.com/help/harmony-25/premium/release-notes/harmony/harmony-25-release-notes.html), [25.1](https://docs.toonboom.com/help/harmony-25/premium/release-notes/harmony/harmony-25-1-release-notes.html) y [25.2](https://docs.toonboom.com/help/harmony-25/premium/release-notes/harmony/harmony-25-2-release-notes.html): actualizaciones incluidas en el corte documental.
- [Ediciones de Harmony](https://helpcentre.toonboom.com/hc/en-ca/articles/48469939119763-What-is-the-difference-between-the-Harmony-Editions): contexto de alcance Premium frente a otras ediciones, sin trasladar sus restricciones comerciales.
- [Storyboard Pro 25](https://docs.toonboom.com/help/storyboard-pro-25/storyboard/index.html): producto separado, excluido del alcance base por decisión del usuario.

Cada capacidad tiene su URL concreta. [reference-index.json](reference-index.json) contiene **1.844 páginas únicas indexadas**. [retrieval-log.json](retrieval-log.json) registra **526 páginas recuperadas por HTTPS**, incluyendo las páginas vinculadas por el catálogo y las referencias de nodos. Recuperar una página no equivale a revisar todos sus parámetros ni a probar la aplicación.

## Tecnología

| Fuente | Base para la decisión |
|---|---|
| [QTabletEvent](https://doc.qt.io/qt-6/qtabletevent.html) | Modelo de eventos de tableta |
| [Qt Quick Scene Graph](https://doc.qt.io/qt-6/qtquick-visualcanvas-scenegraph.html) | Render de la interfaz y separación del motor propio |
| [QQuickRhiItem](https://doc.qt.io/qt-6/qquickrhiitem.html) | Integración de render personalizado y separación entre hilos |
| [QRhi](https://doc.qt.io/qt-6/qrhi.html) | APIs gráficas y garantías limitadas de compatibilidad |
| [Personalización de Qt Quick Controls](https://doc.qt.io/qt-6/qtquickcontrols-customize.html) | Posibilidad de diseño visual propio |
| [Basic Style](https://doc.qt.io/qt-6/qtquickcontrols-basic.html) | Base ligera de estilo de controles |
| [Tauri: arquitectura](https://v2.tauri.app/concept/architecture/) | Shell webview y núcleo Rust |
| [Tauri: IPC](https://v2.tauri.app/concept/inter-process-communication/) | Frontera de comunicación entre procesos |
| [Electron: procesos](https://www.electronjs.org/docs/latest/tutorial/process-model) | Modelo basado en Chromium |
| [wgpu](https://wgpu.rs/) | Alternativa portable de acceso GPU |
| [Skia API](https://skia.org/docs/user/api/) | Candidato de render 2D y geometría |
| [SQLite: atomic commit](https://sqlite.org/atomiccommit.html) | Durabilidad y límites de una transacción de base de datos |
| [OpenColorIO](https://opencolorio.org/) | Pipeline de color para producción |
| [OpenTimelineIO](https://opentimelineio.readthedocs.io/en/latest/) | Intercambio editorial y referencias a medios |
| [OpenFX](https://github.com/AcademySoftwareFoundation/openfx) | Estándar de plugins de efectos |
| [CMake presets](https://cmake.org/cmake/help/latest/manual/cmake-presets.7.html) | Configuraciones de build compartidas |
| [Conan: versiones](https://docs.conan.io/2/tutorial/consuming_packages/intro_to_versioning.html) | Dependencias de C/C++ reproducibles |

Las comparaciones de adecuación y el diseño de OPEN-TOON son análisis propio, no afirmaciones de rendimiento publicadas por estas fuentes.

## Software libre comparable

| Fuente | Uso en el análisis |
|---|---|
| [OpenToonz](https://opentoonz.github.io/e/) | Alternativa existente, alcance y consideración de reutilización |
| [OpenToonz Plastic](https://opentoonz.readthedocs.io/en/latest/create_animations_using_plastic_tool.html) | Referencia funcional de deformación libre |
| [Krita: animación](https://docs.krita.org/en/user_manual/animation.html) | Flujo de animación raster y memoria |
| [Blender Grease Pencil, manual 4.2](https://docs.blender.org/manual/en/4.2/grease_pencil/animation/introduction.html) | Referencia histórica fijada de dibujo animado en 3D; no inventario completo del Blender actual |

## Diseño, accesibilidad y licencias

- [Geist: sistema](https://vercel.com/geist/stack), [colores](https://vercel.com/geist/colors) y [tipografía](https://vercel.com/geist/typography): referencia visual.
- [Geist OFL](https://github.com/vercel/geist-font/blob/main/OFL.txt): licencia de fuente; no licencia general sobre toda la identidad visual de Vercel.
- [WCAG 2.2](https://www.w3.org/TR/WCAG22/): guía de contraste, teclado, foco y acciones de puntero.
- [GPL v3 en OSI](https://opensource.org/license/gpl-3.0) y [texto de GNU](https://www.gnu.org/licenses/gpl-3.0.txt): licencia inicial del repositorio.
- [Qt: licencias](https://doc.qt.io/qt-6/licensing.html): comprobar módulos concretos al distribuir.
- [Skia: licencia](https://github.com/google/skia/blob/main/LICENSE): procedencia del candidato de renderer.
- [FFmpeg: licencia y distribución](https://ffmpeg.org/legal.html): obligaciones dependen de la configuración y componentes del build.

No se considera esta lista una auditoría final de dependencias: todavía no hay un build de aplicación. Cuando exista, se registrarán versiones concretas y sus avisos en un inventario de terceros.
