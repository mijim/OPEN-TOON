# TIM — Timeline, Xsheet y exposición

[Volver al catálogo](../README.md)

> Vista generada desde `features.json` y `domains.json`; no editar a mano.

**Flujo:** Exponer → ajustar timing → reproducir → revisar hoja.

**Módulo:** `timeline`.

**Entidades:** FrameIndex, RationalRate, ExposureSpan, Marker, Annotation.

**Relaciones:** LYR.

**Riesgo principal:** Un desfase de un frame afecta a sonido, dibujo y render.

## Contrato común

Las mutaciones deben respetar transacciones, undo/redo y persistencia. Las vistas de ayuda no se exportan. Errores, cancelación y datos no soportados deben conservar el último estado válido. Estas son condiciones de OPEN-TOON que se concretarán por operación al implementar.

## TIM-001 — Timeline y Xsheet sincronizadas

Ofrecer dos vistas del mismo modelo temporal, horizontal y vertical.

**Aceptación inicial:** Una edición en Xsheet aparece de inmediato en Timeline.

**Alcance:** `base` · **Nivel:** `core` · **Estado:** `not_started`.

**Referencia:** [H25-511b77c6a8](https://docs.toonboom.com/help/harmony-25/premium/layers/about-layer-column.html) · `documentation_linked`.

## TIM-002 — Crear dibujos y exposiciones

Crear dibujos únicos y referenciarlos desde uno o más intervalos de fotogramas.

**Aceptación inicial:** Dos celdas del mismo dibujo reflejan una edición compartida.

**Alcance:** `base` · **Nivel:** `core` · **Estado:** `not_started`.

**Referencia:** [H25-06ae4f88c1](https://docs.toonboom.com/help/harmony-25/premium/timing/about-creating-exposure.html) · `documentation_linked`.

## TIM-003 — Mantener y extender exposición

Alargar o acortar holds sin duplicar los datos del dibujo.

**Aceptación inicial:** Extender un hold diez frames no crea diez dibujos.

**Alcance:** `base` · **Nivel:** `core` · **Estado:** `not_started`.

**Referencia:** [H25-a004f6f168](https://docs.toonboom.com/help/harmony-25/premium/timing/about-adjusting-exposure.html) · `documentation_linked`.

## TIM-004 — Insertar y quitar frames

Modificar duración desplazando rangos según una regla explícita.

**Aceptación inicial:** Insertar dos frames desplaza marcadores y contenido según la opción elegida.

**Alcance:** `base` · **Nivel:** `core` · **Estado:** `not_started`.

**Referencia:** [H25-16442580e1](https://docs.toonboom.com/help/harmony-25/premium/timing/about-scene-length.html) · `documentation_linked`.

## TIM-005 — Vaciar exposición

Quitar una referencia temporal conservando el dibujo disponible.

**Aceptación inicial:** El dibujo se puede volver a exponer desde la biblioteca.

**Alcance:** `base` · **Nivel:** `core` · **Estado:** `not_started`.

**Referencia:** [H25-11f51e512d](https://docs.toonboom.com/help/harmony-25/premium/timing/about-exposure.html) · `documentation_linked`.

## TIM-006 — Copiar y pegar rangos

Distinguir pegado de exposición, dibujos, keyframes y combinaciones.

**Aceptación inicial:** Pegar exposición no añade keyframes de transformación.

**Alcance:** `base` · **Nivel:** `core` · **Estado:** `not_started`.

**Referencia:** [H25-00e4796695](https://docs.toonboom.com/help/harmony-25/premium/timing/about-tweaking-exposure-timeline.html) · `documentation_linked`.

## TIM-007 — Repeticiones y ciclos

Repetir secuencias con dibujos compartidos y límite temporal explícito.

**Aceptación inicial:** Corregir el dibujo original corrige sus repeticiones vinculadas.

**Alcance:** `base` · **Nivel:** `core` · **Estado:** `not_started`.

**Referencia:** [H25-06ae4f88c1](https://docs.toonboom.com/help/harmony-25/premium/timing/about-creating-exposure.html) · `documentation_linked`.

## TIM-008 — Timing a doses y treses

Aplicar pasos de exposición regulares con conservación del orden.

**Aceptación inicial:** Una secuencia de cuatro dibujos a doses ocupa ocho frames.

**Alcance:** `base` · **Nivel:** `core` · **Estado:** `not_started`.

**Referencia:** [H25-a004f6f168](https://docs.toonboom.com/help/harmony-25/premium/timing/about-adjusting-exposure.html) · `documentation_linked`.

## TIM-009 — Arrastrar, estirar y sobrescribir

Mover bloques con opciones de inserción y sobrescritura controladas.

**Aceptación inicial:** El editor muestra qué rango será sustituido antes de soltar.

**Alcance:** `base` · **Nivel:** `pro` · **Estado:** `not_started`.

**Referencia:** [H25-62b9c36095](https://docs.toonboom.com/help/harmony-25/premium/timing/overwrite-exposure-keyframe.html) · `documentation_linked`.

## TIM-010 — Navegación entre dibujos

Saltar a dibujo distinto, keyframe o límite del rango.

**Aceptación inicial:** En un hold, siguiente dibujo salta al siguiente cambio real.

**Alcance:** `base` · **Nivel:** `core` · **Estado:** `not_started`.

**Referencia:** [H25-f87f5edfa6](https://docs.toonboom.com/help/harmony-25/premium/timing/about-timing.html) · `documentation_linked`.

## TIM-011 — Marcadores de escena

Crear marcadores y rangos nombrados con duración y notas.

**Aceptación inicial:** Un marcador bloqueado no se desplaza al arrastrar la regla.

**Alcance:** `base` · **Nivel:** `pro` · **Estado:** `not_started`.

**Referencia:** [H25-f99a3e0bac](https://docs.toonboom.com/help/harmony-25/premium/timing/about-scene-marker.html) · `documentation_linked`.

## TIM-012 — Marcas de fotograma

Clasificar claves, breakdowns e intercalados con símbolos legibles.

**Aceptación inicial:** Los símbolos siguen siendo distinguibles en monocromo.

**Alcance:** `base` · **Nivel:** `pro` · **Estado:** `not_started`.

**Referencia:** [H25-ef6cfd840b](https://docs.toonboom.com/help/harmony-25/premium/timing/about-frame-markers.html) · `documentation_linked`.

## TIM-013 — Miniaturas de exposición

Mostrar previews de dibujos con caché y actualización selectiva.

**Aceptación inicial:** Cambiar un dibujo invalida solo sus miniaturas asociadas.

**Alcance:** `base` · **Nivel:** `pro` · **Estado:** `not_started`.

**Referencia:** [H25-3343e310ae](https://docs.toonboom.com/help/harmony-25/premium/paperless-animation/display-timeline-thumbnails.html) · `documentation_linked`.

## TIM-014 — Imprimir Xsheet

Exportar o imprimir hoja con columnas, tiempos, numeración y anotaciones.

**Aceptación inicial:** Una hoja de varios folios repite cabeceras y no omite frames.

**Alcance:** `base` · **Nivel:** `pro` · **Estado:** `not_started`.

**Referencia:** [H25-2c0f63c654](https://docs.toonboom.com/help/harmony-25/premium/timing/print-xsheet.html) · `documentation_linked`.
