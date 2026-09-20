# Selección tecnológica

## Recomendación

Construir una **aplicación de escritorio nativa con C++20, Qt 6 y Qt Quick/QML**, con bibliotecas de dominio independientes de Qt. Priorizar Windows, macOS y Linux; validar una plataforma primero, pero compilar y comprobar el resto desde que exista el primer núcleo utilizable. La distribución web no es un requisito del usuario y no debe limitar el motor.

La recomendación es un juicio de ingeniería basado en requisitos y documentación, **no un benchmark realizado**. El motor gráfico definitivo queda condicionado a las pruebas SP-01 y SP-02. Los núcleos difíciles son el trazo, la topología de regiones pintables, la deformación, el compositor y el guardado; ningún framework los proporciona como un equivalente listo de Harmony.

## Criterios

| Criterio | Peso orientativo | Por qué importa |
|---|---:|---|
| Entrada de lápiz y adaptación al escritorio | 25 % | Presión, inclinación, captura, HiDPI y múltiples pantallas |
| Motor gráfico y cómputo | 25 % | Geometría compleja, raster, deformación y composición |
| Interoperabilidad profesional | 15 % | Librerías C/C++, audio, imágenes, color, códecs y plugins |
| Mantenibilidad y extensibilidad | 15 % | Separación de dominio, pruebas y contribuciones |
| Construcción de UI y productividad | 10 % | Densidad de editor, accesibilidad y diseño propio |
| Portabilidad y distribución | 10 % | Tres sistemas y dependencias instalables |

Pesos propuestos para este producto. No se asignan puntuaciones numéricas que aparenten mediciones inexistentes.

## Comparación de alternativas

| Opción | Ventajas para OPEN-TOON | Costes y riesgos | Dictamen |
|---|---|---|---|
| C++20 + Qt Quick | Eventos nativos, ecosistema gráfico, interoperabilidad directa, UI declarativa personalizable | Mayor disciplina de memoria; docking avanzado y controles de animación requieren desarrollo; integración GPU exigente | Recomendada |
| C++20 + Qt Widgets | Docking y controles de escritorio consolidados; ruta pragmática para equipos Qt | Estilo fino y animaciones visuales más manuales; widgets no resuelven motor gráfico | Alternativa si QML falla en docking o accesibilidad |
| Rust + Tauri + React | Rust para dominio, UI web productiva, tamaño de shell contenido | Diferencias entre webviews; ruta lápiz/render y transferencia de datos necesitan diseño; host de SDKs C++ añade FFI | Válida si se prioriza equipo web y supera los spikes |
| Electron + React + núcleo nativo | Chromium homogéneo entre sistemas, UI web y herramientas de diagnóstico | Memoria y distribución mayores; frontera con motor nativo, versiones de runtime y módulos | Alternativa al enfoque web, no preferida para esta base |
| Navegador + WebGPU/WASM | Acceso sin instalar y facilidad de compartir enlaces | Compatibilidad de navegador, filesystem, codecs, workers y recursos; menor control del entorno de tableta | Futuro visor/revisor, no editor principal inicial |
| Rust + wgpu + UI nativa | Buen control de ownership y biblioteca GPU portable | Herramientas de escritorio/artist workflow y enlaces con ecosistema DCC requieren trabajo adicional | Buena alternativa de motor, no introducir como segundo lenguaje sin necesidad |
| Python + PySide | Prototipado de UI y ecosistema técnico accesible | Trabajo intensivo y grandes estructuras necesitan capa nativa; dos lenguajes y empaquetado | Herramientas de pipeline y prototipos, no núcleo del render |

[Qt documenta eventos de tableta con presión e inclinación](https://doc.qt.io/qt-6/qtabletevent.html). Esto no garantiza que cualquier driver o dispositivo funcione correctamente: las combinaciones reales se prueban. [Qt Quick permite personalizar controles](https://doc.qt.io/qt-6/qtquickcontrols-customize.html); por tanto, el estilo Geist no exige React. [Tauri separa webview y Rust mediante IPC](https://v2.tauri.app/concept/inter-process-communication/), [Electron utiliza el modelo multiproceso de Chromium](https://www.electronjs.org/docs/latest/tutorial/process-model) y [wgpu ofrece una abstracción gráfica portable](https://wgpu.rs/). Estas son características documentadas; las valoraciones de la tabla son inferencias para este proyecto.

## Construir, reutilizar o bifurcar

No conviene ignorar alternativas libres existentes. [OpenToonz](https://opentoonz.github.io/e/) ya cubre raster/vector, exposición, composición y un ecosistema de efectos; su [herramienta Plastic](https://opentoonz.readthedocs.io/en/latest/create_animations_using_plastic_tool.html) trabaja con deformación por malla/esqueleto. Un fork podría adelantar parte del producto, pero adaptar sus modelos, UI y arquitectura tiene coste y compromete el control del diseño. Su código principal indica BSD; eso no autoriza a asumir que todas las dependencias y recursos tengan esa licencia.

[Krita](https://docs.krita.org/en/user_manual/animation.html) es una referencia valiosa de pintura y animación raster. Su manual distingue ese foco de un montaje audiovisual extenso. [Blender Grease Pencil](https://docs.blender.org/manual/en/4.2/grease_pencil/animation/introduction.html) combina dibujo animado y espacio 3D; su producto completo impone un modelo mental distinto del editor especializado que buscamos.

**Propuesta:** desarrollar un núcleo propio, reutilizar bibliotecas genéricas maduras e investigar componentes concretos antes de reescribir algoritmos. Antes del plan largo, ejecutar SP-00: una auditoría limitada de OpenToonz para estimar qué compensa reutilizar y qué acoplamientos implicaría. No se ha auditado su código en esta etapa; la decisión de no hacer un fork todavía es provisional. La alternativa de adaptar OpenToonz deberá reconsiderarse si recursos humanos o plazos hacen inviable el alcance propio.

## Pila por responsabilidad

| Responsabilidad | Propuesta | Condición |
|---|---|---|
| Dominio, comandos y evaluación | C++20, STL, RAII, value types | Sin Qt ni globales de aplicación en el dominio |
| UI | Qt 6 Quick/QML, controles propios sobre Basic | Modelo-vista, navegación accesible y sin lógica de documento en QML |
| Motor vectorial | Geometría propia + candidato Skia para rasterización/paths | Evaluar regiones, cobertura, alfa y preservación de datos editables |
| Backend GPU | Adaptador Qt RHI; integración de superficies Skia como alternativa experimental | No presuponer interoperabilidad zero-copy |
| Color | OpenColorIO con configuración de producción versionada | Separar espacio de trabajo y transformación de pantalla |
| Imágenes | Adaptadores PNG/EXR y candidato OpenImageIO | Revisar codecs y licencias realmente incluidos |
| Audio/codificación | FFmpeg como proceso/adapter; reproducción con motor de audio acotado | Licencias por build, reloj de audio, cancelación y perfiles explícitos |
| Proyecto local | SQLite para metadatos/revisiones + blobs inmutables por hash | Diseño de commit que abarque archivos y DB, no solo transacción SQL |
| Intercambio editorial | OpenTimelineIO como adaptador opcional | No transporta la semántica completa de un rig ni un compositor |
| Automatización | CLI del núcleo y Python mediante API acotada, más adelante | No depender de scripts para operaciones esenciales de edición |
| Plugins | API interna primero; OpenFX como adapter posterior | Perfil de host, procesos y versiones definidos |
| Build | CMake presets + Ninja; Conan 2 como candidato para dependencias | Versiones y hashes fijados; no combinar gestores sin motivo |
| Pruebas | Tests C++ de dominio; Qt Test/Quick Test para UI; fixtures de imagen y proyectos | Elegir bibliotecas exactas en el primer scaffold |

[Skia](https://skia.org/docs/user/api/) ayuda a dibujar geometría, pero no aporta un modelo de animación, un editor de curvas ni la semántica de rellenos de Harmony. [OpenColorIO](https://opencolorio.org/) se orienta al color en producción audiovisual. [OpenTimelineIO](https://opentimelineio.readthedocs.io/en/latest/) describe montaje y referencias a medios; no sustituye nuestro formato de escena.

## Decisión gráfica que no debe ocultarse

Qt Quick tiene su propio [scene graph de UI](https://doc.qt.io/qt-6/qtquick-visualcanvas-scenegraph.html). El compositor de OPEN-TOON será otro modelo, evaluado por el motor y presentado como imagen/superficie en el viewport.

[QQuickRhiItem](https://doc.qt.io/qt-6/qquickrhiitem.html) permite integrar render basado en RHI con Qt Quick. Sin embargo, **QRhi tiene garantías limitadas de compatibilidad fuente y binaria**, requiere APIs asociadas a `Qt::GuiPrivate`, y el backend software de Qt Quick no hace funcionar automáticamente ese item. Debe aislarse en un adapter, fijar la versión de Qt y disponer de una ruta CPU de diagnóstico independiente. No se prometerá compatibilidad con una actualización de Qt sin compilar y verificar ese adapter.

Dos candidatos para SP-02: rasterización CPU de referencia más upload de regiones dañadas, y pipeline GPU mediante RHI. Skia se evalúa como implementación del renderer vectorial, no como tercer modelo de documento. La UI no debe recibir imágenes PNG ni grandes JSON por cada movimiento del lápiz.

## Versiones y licencias

Fijar versiones estables concretas **al comenzar la implementación**, con lockfiles, compiladores y matriz de sistemas comprobados. No confundir documentación `/latest/` con una versión de dependencia seleccionada. [CMake presets](https://cmake.org/cmake/help/latest/manual/cmake-presets.7.html) permite compartir configuraciones y [Conan](https://docs.conan.io/2/tutorial/consuming_packages/intro_to_versioning.html) documenta la gestión de versiones de paquetes.

La publicación inicial adopta **GPL-3.0-or-later** para mantener abiertas las modificaciones redistribuidas del proyecto. Una licencia permisiva facilitaría otros tipos de integración, pero permitiría derivados cerrados; esta diferencia se registra para revisión del propietario antes de aceptar contribuciones significativas. La [licencia GPL v3](https://opensource.org/license/gpl-3.0) y el [licenciamiento de Qt](https://doc.qt.io/qt-6/licensing.html) son las referencias de partida. Qt ofrece licencias diferentes según módulo; no se presupone una licencia uniforme para todo Qt.

FFmpeg requiere examinar las opciones exactas del build, bibliotecas externas, avisos y obligaciones de redistribución; su propia [guía legal](https://ffmpeg.org/legal.html) distingue combinaciones. Ejecutarlo como proceso externo no elimina automáticamente esas obligaciones. También se revisarán las licencias de plugins y de los modelos opcionales. Esto forma parte de la selección técnica de dependencias, no de una promesa de compatibilidad jurídica universal.
