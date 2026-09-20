# RAS — Dibujo bitmap, texturas y pinceles

[Volver al catálogo](../README.md)

> Vista generada desde `features.json` y `domains.json`; no editar a mano.

**Flujo:** Dibujar bitmap → editar → gestionar resolución → intercambiar pinceles.

**Módulo:** `drawing-raster`.

**Entidades:** RasterTile, BrushPreset, TextureAsset, RasterSelection.

**Relaciones:** LYR, UI.

**Riesgo principal:** Consumo de memoria y coherencia entre textura y transformación.

## Contrato común

Las mutaciones deben respetar transacciones, undo/redo y persistencia. Las vistas de ayuda no se exportan. Errores, cancelación y datos no soportados deben conservar el último estado válido. Estas son condiciones de OPEN-TOON que se concretarán por operación al implementar.

## RAS-001 — Capas bitmap

Pintar imágenes por píxeles con resolución definida y transparencia.

**Aceptación inicial:** Una capa vacía exporta alfa cero.

**Alcance:** `base` · **Nivel:** `core` · **Estado:** `not_started`.

**Referencia:** [H25-244ec6be5c](https://docs.toonboom.com/help/harmony-25/premium/drawing/about-bitmap-layers.html) · `documentation_linked`.

## RAS-002 — Pincel bitmap texturado

Soportar punta, espaciado, opacidad y dinámica en trazos raster.

**Aceptación inicial:** Un trazo largo mantiene densidad estable al variar velocidad.

**Alcance:** `base` · **Nivel:** `core` · **Estado:** `not_started`.

**Referencia:** [H25-d57641e891](https://docs.toonboom.com/help/harmony-25/premium/drawing/about-brush-tool.html) · `documentation_linked`.

## RAS-003 — Goma bitmap

Eliminar o reducir alfa con borde y opacidad configurables.

**Aceptación inicial:** Borrar no introduce píxeles negros opacos en el borde.

**Alcance:** `base` · **Nivel:** `core` · **Estado:** `not_started`.

**Referencia:** [H25-fbeaac1886](https://docs.toonboom.com/help/harmony-25/premium/drawing/about-eraser-tool.html) · `documentation_linked`.

## RAS-004 — Resolución de textura

Modificar resolución y calidad de imágenes explicando la pérdida por remuestreo.

**Aceptación inicial:** Escalar hacia abajo conserva dimensiones físicas acordadas.

**Alcance:** `base` · **Nivel:** `pro` · **Estado:** `not_started`.

**Referencia:** [H25-b18995cc34](https://docs.toonboom.com/help/harmony-25/premium/import/change-bitmap-image-quality.html) · `documentation_linked`.

## RAS-005 — Selección raster

Transformar píxeles seleccionados conservando máscara y canal alfa.

**Aceptación inicial:** Rotar la selección no rellena el exterior con un color sólido.

**Alcance:** `base` · **Nivel:** `core` · **Estado:** `not_started`.

**Referencia:** [H25-0441f46367](https://docs.toonboom.com/help/harmony-25/premium/drawing/about-select-tool.html) · `documentation_linked`.

## RAS-006 — Pinceles ABR

Importar el subconjunto soportado de pinceles Photoshop y emitir un informe de parámetros ignorados.

**Aceptación inicial:** Un ABR no compatible se rechaza con diagnóstico sin crear un preset roto.

**Alcance:** `base` · **Nivel:** `pro` · **Estado:** `not_started`.

**Referencia:** [H25-e7577ec800](https://docs.toonboom.com/help/harmony-25/premium/release-notes/harmony/harmony-25-release-notes.html) · `documentation_linked`.

## RAS-007 — Pincel vectorial con textura bitmap

Mantener separadas geometría vectorial y textura raster dependiente de resolución.

**Aceptación inicial:** El editor advierte cuando la ampliación excede resolución de textura.

**Alcance:** `base` · **Nivel:** `pro` · **Estado:** `not_started`.

**Referencia:** [H25-d57641e891](https://docs.toonboom.com/help/harmony-25/premium/drawing/about-brush-tool.html) · `documentation_linked`.
