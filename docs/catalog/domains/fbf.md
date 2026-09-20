# FBF — Animación tradicional y paperless

[Volver al catálogo](../README.md)

> Vista generada desde `features.json` y `domains.json`; no editar a mano.

**Flujo:** Rough → tie-down → cleanup → pintura → revisión.

**Módulo:** `animation-drawing`.

**Entidades:** OnionSettings, TraceOffset, DrawingMark, DrawingDesk.

**Relaciones:** TIM, VEC, COL.

**Riesgo principal:** Las ayudas visuales nunca deben convertirse accidentalmente en arte exportado.

## Contrato común

Las mutaciones deben respetar transacciones, undo/redo y persistencia. Las vistas de ayuda no se exportan. Errores, cancelación y datos no soportados deben conservar el último estado válido. Estas son condiciones de OPEN-TOON que se concretarán por operación al implementar.

## FBF-001 — Dibujo frame a frame

Crear y recorrer dibujos con continuidad de herramientas y exposición.

**Aceptación inicial:** Dibujar en un nuevo frame no modifica el anterior sin intención explícita.

**Alcance:** `base` · **Nivel:** `core` · **Estado:** `not_started`.

**Referencia:** [H25-35cb7c7eb7](https://docs.toonboom.com/help/harmony-25/premium/paperless-animation/about-paperless-animation.html) · `documentation_linked`.

## FBF-002 — Rough, tie-down y cleanup

Organizar fases de dibujo mediante capas, marcas y visibilidad.

**Aceptación inicial:** Ocultar rough deja un cleanup completo sin referencias perdidas.

**Alcance:** `base` · **Nivel:** `core` · **Estado:** `not_started`.

**Referencia:** [H25-4171e1f348](https://docs.toonboom.com/help/harmony-25/premium/paperless-animation/clean-animation.html) · `documentation_linked`.

## FBF-003 — Onion skin anterior y posterior

Mostrar dibujos vecinos con rango, opacidad y diferenciación configurable.

**Aceptación inicial:** El onion skin no aparece en el render final.

**Alcance:** `base` · **Nivel:** `core` · **Estado:** `not_started`.

**Referencia:** [H25-1b726e005c](https://docs.toonboom.com/help/harmony-25/premium/paperless-animation/about-onion-skin.html) · `documentation_linked`.

## FBF-004 — Onion skin avanzado

Seleccionar dibujos concretos, separar frames de dibujos únicos y comparar capas.

**Aceptación inicial:** Un hold no se cuenta varias veces al operar por dibujos únicos.

**Alcance:** `base` · **Nivel:** `pro` · **Estado:** `not_started`.

**Referencia:** [H25-788d9faecc](https://docs.toonboom.com/help/harmony-25/premium/paperless-animation/about-advanced-onion-skin.html) · `documentation_linked`.

## FBF-005 — Onion skin entre capas

Comparar referencias de otras capas con reglas de visibilidad.

**Aceptación inicial:** Bloquear una capa no impide usarla como referencia.

**Alcance:** `base` · **Nivel:** `pro` · **Estado:** `not_started`.

**Referencia:** [H25-e7cef9ee88](https://docs.toonboom.com/help/harmony-25/premium/paperless-animation/display-onion-skin-other-layer.html) · `documentation_linked`.

## FBF-006 — Shift and trace

Mover y rotar temporalmente dibujos de referencia para calcar.

**Aceptación inicial:** Restablecer la referencia no cambia los datos fuente.

**Alcance:** `base` · **Nivel:** `pro` · **Estado:** `not_started`.

**Referencia:** [H25-6b716a0ca1](https://docs.toonboom.com/help/harmony-25/premium/paperless-animation/about-shift-trace.html) · `documentation_linked`.

## FBF-007 — Mesa de dibujos

Organizar dibujos de referencia fuera de su exposición temporal.

**Aceptación inicial:** Retirar una referencia de la mesa no elimina su dibujo.

**Alcance:** `base` · **Nivel:** `pro` · **Estado:** `not_started`.

**Referencia:** [H25-f9e8084820](https://docs.toonboom.com/help/harmony-25/premium/paperless-animation/about-drawing-desk.html) · `documentation_linked`.

## FBF-008 — Flip y reproducción corta

Alternar poses y reproducir un segmento corto sin perder selección.

**Aceptación inicial:** Salir de mini playback recupera el frame de trabajo.

**Alcance:** `base` · **Nivel:** `core` · **Estado:** `not_started`.

**Referencia:** [H25-638f2d31ee](https://docs.toonboom.com/help/harmony-25/premium/paperless-animation/use-mini-playback.html) · `documentation_linked`.

## FBF-009 — Marcas de dibujo

Marcar dibujos como claves, breakdowns u otras categorías personalizadas.

**Aceptación inicial:** Una marca de dibujo se refleja en todas sus exposiciones.

**Alcance:** `base` · **Nivel:** `pro` · **Estado:** `not_started`.

**Referencia:** [H25-b8f3953dbe](https://docs.toonboom.com/help/harmony-25/premium/paperless-animation/about-mark-drawing.html) · `documentation_linked`.

## FBF-010 — Generar Colour Art desde línea

Construir fronteras de pintura derivadas de Line Art con tolerancias.

**Aceptación inicial:** Regenerar mantiene posibilidad de conservar correcciones manuales.

**Alcance:** `base` · **Nivel:** `pro` · **Estado:** `not_started`.

**Referencia:** [H25-e846aeb2c3](https://docs.toonboom.com/help/harmony-25/premium/paperless-animation/create-colour-art-line-art.html) · `documentation_linked`.

## FBF-011 — Generar matte

Producir siluetas de referencia a partir de dibujos.

**Aceptación inicial:** La silueta conserva huecos según la opción configurada.

**Alcance:** `base` · **Nivel:** `pro` · **Estado:** `not_started`.

**Referencia:** [H25-dfaa873304](https://docs.toonboom.com/help/harmony-25/premium/paperless-animation/generate-matte.html) · `documentation_linked`.

## FBF-012 — Registro y cruces

Usar marcas de alineación que facilitan el registro entre dibujos.

**Aceptación inicial:** Las cruces de ayuda no se renderizan como arte final.

**Alcance:** `base` · **Nivel:** `pro` · **Estado:** `not_started`.

**Referencia:** [H25-2d2d07f3bf](https://docs.toonboom.com/help/harmony-25/premium/paperless-animation/add-cross-hairs.html) · `documentation_linked`.
