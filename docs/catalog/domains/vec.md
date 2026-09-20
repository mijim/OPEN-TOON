# VEC — Dibujo vectorial y edición geométrica

[Volver al catálogo](../README.md)

> Vista generada desde `features.json` y `domains.json`; no editar a mano.

**Flujo:** Boceto → selección → limpieza → edición del contorno → dibujo final.

**Módulo:** `drawing-vector`.

**Entidades:** VectorStroke, BezierPath, WidthProfile, FillRegion, Selection.

**Relaciones:** LYR, UI.

**Riesgo principal:** Topología, precisión y calidad del trazo son trabajo de motor.

## Contrato común

Las mutaciones deben respetar transacciones, undo/redo y persistencia. Las vistas de ayuda no se exportan. Errores, cancelación y datos no soportados deben conservar el último estado válido. Estas son condiciones de OPEN-TOON que se concretarán por operación al implementar.

## VEC-001 — Lápiz de línea central

Representar trazos como curvas con grosor editable y extremos configurables.

**Aceptación inicial:** Cambiar grosor no desplaza la línea central.

**Alcance:** `base` · **Nivel:** `core` · **Estado:** `not_started`.

**Referencia:** [H25-616e4225a1](https://docs.toonboom.com/help/harmony-25/premium/drawing/about-pencil-tool.html) · `documentation_linked`.

## VEC-002 — Pincel vectorial

Crear siluetas rellenadas editables desde gestos de dibujo.

**Aceptación inicial:** Un trazo cerrado sigue siendo editable por puntos.

**Alcance:** `base` · **Nivel:** `core` · **Estado:** `not_started`.

**Referencia:** [H25-d57641e891](https://docs.toonboom.com/help/harmony-25/premium/drawing/about-brush-tool.html) · `documentation_linked`.

## VEC-003 — Grosor variable

Aplicar presión y perfiles variables a lo largo del trazo.

**Aceptación inicial:** Los extremos finos se conservan al ampliar la vista.

**Alcance:** `base` · **Nivel:** `core` · **Estado:** `not_started`.

**Referencia:** [H25-61b6ece53c](https://docs.toonboom.com/help/harmony-25/premium/drawing/about-pencil-editor-tool.html) · `documentation_linked`.

## VEC-004 — Textura de lápiz

Asociar textura y parámetros de repetición a una línea central.

**Aceptación inicial:** Cambiar la longitud mantiene continuidad de textura definida.

**Alcance:** `base` · **Nivel:** `pro` · **Estado:** `not_started`.

**Referencia:** [H25-e34ac985dd](https://docs.toonboom.com/help/harmony-25/premium/drawing/about-pencil-line-texture.html) · `documentation_linked`.

## VEC-005 — Presets de pincel

Guardar tamaño, punta, suavizado y textura como un preset reutilizable.

**Aceptación inicial:** Exportar e importar el preset conserva el aspecto de una muestra.

**Alcance:** `base` · **Nivel:** `core` · **Estado:** `not_started`.

**Referencia:** [H25-a91371a694](https://docs.toonboom.com/help/harmony-25/premium/drawing/about-brush-presets.html) · `documentation_linked`.

## VEC-006 — Presets de lápiz

Guardar perfiles y configuración de líneas con identidad propia.

**Aceptación inicial:** Cambiar de preset no modifica trazos ya existentes.

**Alcance:** `base` · **Nivel:** `core` · **Estado:** `not_started`.

**Referencia:** [H25-5f58963884](https://docs.toonboom.com/help/harmony-25/premium/drawing/about-pencil-presets.html) · `documentation_linked`.

## VEC-007 — Estabilizador

Suavizar ruido del lápiz mediante parámetros de retardo y estabilización.

**Aceptación inicial:** El final del trazo alcanza el punto levantado sin segmento espurio.

**Alcance:** `base` · **Nivel:** `core` · **Estado:** `not_started`.

**Referencia:** [H25-01d0620968](https://docs.toonboom.com/help/harmony-25/premium/drawing/about-stabilization.html) · `documentation_linked`.

## VEC-008 — Goma vectorial

Recortar geometría atravesada por una goma con tamaño y presión.

**Aceptación inicial:** Borrar parcialmente una línea deja dos segmentos válidos.

**Alcance:** `base` · **Nivel:** `core` · **Estado:** `not_started`.

**Referencia:** [H25-fbeaac1886](https://docs.toonboom.com/help/harmony-25/premium/drawing/about-eraser-tool.html) · `documentation_linked`.

## VEC-009 — Selector y lazo

Seleccionar trazos o regiones, sumar y restar selección y transformarla.

**Aceptación inicial:** Una selección parcial no mueve trazos externos al área elegida.

**Alcance:** `base` · **Nivel:** `core` · **Estado:** `not_started`.

**Referencia:** [H25-0441f46367](https://docs.toonboom.com/help/harmony-25/premium/drawing/about-select-tool.html) · `documentation_linked`.

## VEC-010 — Editor de contornos

Añadir, eliminar y mover puntos y tangentes de curvas.

**Aceptación inicial:** Eliminar un punto mantiene una curva válida y deshacer la restaura.

**Alcance:** `base` · **Nivel:** `core` · **Estado:** `not_started`.

**Referencia:** [H25-2f4c000b7c](https://docs.toonboom.com/help/harmony-25/premium/drawing/about-contour-editor-tool.html) · `documentation_linked`.

## VEC-011 — Editor de línea central

Editar la trayectoria central preservando el aspecto del pincel cuando sea posible.

**Aceptación inicial:** Mover un control no cambia el color de relleno.

**Alcance:** `base` · **Nivel:** `pro` · **Estado:** `not_started`.

**Referencia:** [H25-33351193bd](https://docs.toonboom.com/help/harmony-25/premium/drawing/about-centerline-editor.html) · `documentation_linked`.

## VEC-012 — Editor de grosor

Modificar el perfil del lápiz sin redibujar la trayectoria.

**Aceptación inicial:** Un ajuste localizado de grosor no afecta al resto del trazo.

**Alcance:** `base` · **Nivel:** `pro` · **Estado:** `not_started`.

**Referencia:** [H25-61b6ece53c](https://docs.toonboom.com/help/harmony-25/premium/drawing/about-pencil-editor-tool.html) · `documentation_linked`.

## VEC-013 — Retoque de lápiz

Pintar ajustes de grosor u opacidad con aumento, reducción, sustitución y suavizado.

**Aceptación inicial:** Suavizar opacidad no cambia posiciones de los puntos.

**Alcance:** `base` · **Nivel:** `pro` · **Estado:** `not_started`.

**Referencia:** [H25-d8ff27b34b](https://docs.toonboom.com/help/harmony-25/premium/drawing/pencil-retouch-tool.html) · `documentation_linked`.

## VEC-014 — Suavizado posterior

Reducir irregularidades de contornos con intensidad controlable.

**Aceptación inicial:** El ajuste conserva esquinas protegidas dentro de la tolerancia definida.

**Alcance:** `base` · **Nivel:** `pro` · **Estado:** `not_started`.

**Referencia:** [H25-bc4b2e3785](https://docs.toonboom.com/help/harmony-25/premium/drawing/about-smooth-editor-tool.html) · `documentation_linked`.

## VEC-015 — Líneas y primitivas

Dibujar líneas, rectángulos y elipses con restricciones y relleno opcional.

**Aceptación inicial:** Mantener la restricción produce un círculo geométrico.

**Alcance:** `base` · **Nivel:** `core` · **Estado:** `not_started`.

**Referencia:** [H25-8b70ba5f4a](https://docs.toonboom.com/help/harmony-25/premium/drawing/draw-ellipse-rectangle-line-tool.html) · `documentation_linked`.

## VEC-016 — Polyline y Bézier

Construir curvas precisas por puntos y manejadores.

**Aceptación inicial:** Cerrar un camino crea una región pintable sin hueco residual.

**Alcance:** `base` · **Nivel:** `core` · **Estado:** `not_started`.

**Referencia:** [H25-a22d6d719b](https://docs.toonboom.com/help/harmony-25/premium/drawing/about-shape-tool.html) · `documentation_linked`.

## VEC-017 — Cortador

Separar una región mediante lazo y conservarla como selección editable.

**Aceptación inicial:** Mover el recorte no arrastra el dibujo exterior.

**Alcance:** `base` · **Nivel:** `pro` · **Estado:** `not_started`.

**Referencia:** [H25-a103fb42af](https://docs.toonboom.com/help/harmony-25/premium/drawing/about-cutter-tool.html) · `documentation_linked`.

## VEC-018 — Deformación de perspectiva

Deformar una selección con controles de perspectiva.

**Aceptación inicial:** Deshacer restituye exactamente los puntos anteriores.

**Alcance:** `base` · **Nivel:** `pro` · **Estado:** `not_started`.

**Referencia:** [H25-c6c8383a10](https://docs.toonboom.com/help/harmony-25/premium/drawing/about-perspective-tool.html) · `documentation_linked`.

## VEC-019 — Envelope de dibujo

Editar una envolvente que deforma geometría seleccionada.

**Aceptación inicial:** La envolvente opera solo sobre la selección capturada.

**Alcance:** `base` · **Nivel:** `pro` · **Estado:** `not_started`.

**Referencia:** [H25-c81dc674da](https://docs.toonboom.com/help/harmony-25/premium/drawing/about-envelope-tool.html) · `documentation_linked`.

## VEC-020 — Trazos invisibles

Definir fronteras de relleno que no generan línea visible.

**Aceptación inicial:** La frontera contiene la pintura y no aparece al exportar.

**Alcance:** `base` · **Nivel:** `pro` · **Estado:** `not_started`.

**Referencia:** [H25-313cdd883b](https://docs.toonboom.com/help/harmony-25/premium/drawing/about-stroke-tool.html) · `documentation_linked`.

## VEC-021 — Conversión de trazos

Convertir tipos de línea según restricciones explícitas y conservar una copia recuperable.

**Aceptación inicial:** La conversión informa de textura u opacidad no preservada.

**Alcance:** `base` · **Nivel:** `pro` · **Estado:** `not_started`.

**Referencia:** [H25-51372e7004](https://docs.toonboom.com/help/harmony-25/premium/drawing/about-stroke-conversion.html) · `documentation_linked`.

## VEC-022 — Optimización vectorial

Simplificar geometría y combinar elementos compatibles con tolerancia configurada.

**Aceptación inicial:** La simplificación respeta una tolerancia de error visual publicada.

**Alcance:** `base` · **Nivel:** `pro` · **Estado:** `not_started`.

**Referencia:** [H25-410c7b61f5](https://docs.toonboom.com/help/harmony-25/premium/drawing/about-drawing-optimization.html) · `documentation_linked`.

## VEC-023 — Sellos

Estampar dibujos reutilizables con posición, escala y rotación.

**Aceptación inicial:** Cada sello conserva la transformación con que se insertó.

**Alcance:** `base` · **Nivel:** `pro` · **Estado:** `not_started`.

**Referencia:** [H25-dca5f92aa5](https://docs.toonboom.com/help/harmony-25/premium/drawing/about-stamp-tool.html) · `documentation_linked`.

## VEC-024 — Texto

Crear texto editable y convertirlo a geometría cuando se necesite portabilidad.

**Aceptación inicial:** El texto convertido mantiene su aspecto sin la fuente instalada.

**Alcance:** `base` · **Nivel:** `pro` · **Estado:** `not_started`.

**Referencia:** [H25-ebbe0d6b72](https://docs.toonboom.com/help/harmony-25/premium/drawing/about-text-tool.html) · `documentation_linked`.

## VEC-025 — Guías y rejilla

Mostrar cuadrícula y referencias con ajuste opcional.

**Aceptación inicial:** Desactivar ajuste permite colocar puntos entre líneas de rejilla.

**Alcance:** `base` · **Nivel:** `core` · **Estado:** `not_started`.

**Referencia:** [H25-bf8814a6a5](https://docs.toonboom.com/help/harmony-25/premium/drawing/about-grid.html) · `documentation_linked`.

## VEC-026 — Guías de perspectiva

Asistir dibujo con puntos de fuga, isometría y perspectivas curvas.

**Aceptación inicial:** Mover una guía cambia la asistencia y no los dibujos terminados.

**Alcance:** `base` · **Nivel:** `pro` · **Estado:** `not_started`.

**Referencia:** [H25-b20079c397](https://docs.toonboom.com/help/harmony-25/premium/drawing/about-drawing-guide.html) · `documentation_linked`.

## VEC-027 — Reposicionar todos los dibujos

Aplicar una transformación geométrica a un conjunto de dibujos del elemento.

**Aceptación inicial:** La acción desplaza dibujos usados y no expuestos según selección explícita.

**Alcance:** `base` · **Nivel:** `pro` · **Estado:** `not_started`.

**Referencia:** [H25-1e276f919c](https://docs.toonboom.com/help/harmony-25/premium/drawing/about-reposition-all-drawing-tool.html) · `documentation_linked`.
