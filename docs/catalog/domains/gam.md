# GAM — Salida para videojuegos

[Volver al catálogo](../README.md)

> Vista generada desde `features.json` y `domains.json`; no editar a mano.

**Flujo:** Preparar rig compatible → hornear → exportar → validar runtime.

**Módulo:** `game-export`.

**Entidades:** SpriteAtlas, RuntimeSkeleton, Anchor, CollisionShape.

**Relaciones:** DEF, OUT, LIB.

**Riesgo principal:** No todos los nodos y deformadores tienen representación en motores de juego.

## Contrato común

Las mutaciones deben respetar transacciones, undo/redo y persistencia. Las vistas de ayuda no se exportan. Errores, cancelación y datos no soportados deben conservar el último estado válido. Estas son condiciones de OPEN-TOON que se concretarán por operación al implementar.

## GAM-001 — Sprite sheets

Empaquetar frames con metadatos de pivote, tamaño y temporización.

**Aceptación inicial:** Reconstruir la animación desde atlas conserva registro y timing.

**Alcance:** `base` · **Nivel:** `pro` · **Estado:** `not_started`.

**Referencia:** [H25-817ec6e450](https://docs.toonboom.com/help/harmony-25/premium/gaming/export-sprite-sheet.html) · `documentation_linked`.

## GAM-002 — Esqueleto y animación de runtime

Exportar un perfil de rig compatible con un runtime objetivo.

**Aceptación inicial:** Un clip exportado coincide en las poses de referencia definidas.

**Alcance:** `base` · **Nivel:** `advanced` · **Estado:** `not_started`.

**Referencia:** [H25-942fa9b3c6](https://docs.toonboom.com/help/harmony-25/premium/gaming/about-export-unity.html) · `documentation_linked`.

## GAM-003 — Horneado de efectos

Convertir partes no soportadas en imágenes o grupos precompuestos.

**Aceptación inicial:** El informe identifica los elementos que dejan de ser editables en runtime.

**Alcance:** `base` · **Nivel:** `advanced` · **Estado:** `not_started`.

**Referencia:** [H25-67228ec950](https://docs.toonboom.com/help/harmony-25/premium/gaming/about-baked-effects.html) · `documentation_linked`.

## GAM-004 — Anclas para accesorios

Exportar puntos de montaje y transformaciones animadas.

**Aceptación inicial:** Un accesorio se mantiene unido a la mano en el ejemplo de runtime.

**Alcance:** `base` · **Nivel:** `advanced` · **Estado:** `not_started`.

**Referencia:** [H25-725a89f4f2](https://docs.toonboom.com/help/harmony-25/premium/gaming/set-anchor-prop.html) · `documentation_linked`.

## GAM-005 — Colisiones y metadatos

Asociar regiones de colisión y datos de juego a la exportación.

**Aceptación inicial:** Las colisiones respetan escala y origen del asset.

**Alcance:** `base` · **Nivel:** `advanced` · **Estado:** `not_started`.

**Referencia:** [H25-32924fa833](https://docs.toonboom.com/help/harmony-25/premium/gaming/set-up-collisions-unity.html) · `documentation_linked`.

## GAM-006 — Variaciones de paleta en runtime

Representar variantes compatibles o generar atlas por variante.

**Aceptación inicial:** La variante seleccionada no cambia el timing de animación.

**Alcance:** `base` · **Nivel:** `advanced` · **Estado:** `not_started`.

**Referencia:** [H25-c7c5b24d9a](https://docs.toonboom.com/help/harmony-25/premium/gaming/palette-variations.html) · `documentation_linked`.

## GAM-007 — Salida HTML heredada

Registrar exportación EaselJS como compatibilidad histórica de referencia.

**Aceptación inicial:** El análisis distingue este formato del formato nativo OPEN-TOON.

**Alcance:** `legacy_candidate` · **Nivel:** `legacy` · **Estado:** `not_started`.

**Referencia:** [H25-5f3db170a0](https://docs.toonboom.com/help/harmony-25/premium/gaming/export-easel-js.html) · `documentation_linked`.
