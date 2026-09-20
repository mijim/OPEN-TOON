# AUD — Sonido y sincronización labial

[Volver al catálogo](../README.md)

> Vista generada desde `features.json` y `domains.json`; no editar a mano.

**Flujo:** Importar sonido → ajustar → escuchar → detectar y corregir bocas.

**Módulo:** `audio`.

**Entidades:** AudioAsset, AudioClip, AudioTrack, PhonemeTrack, VisemeMap.

**Relaciones:** TIM, RIG.

**Riesgo principal:** Deriva audiovisual, cortes de audio y detección imperfecta.

## Contrato común

Las mutaciones deben respetar transacciones, undo/redo y persistencia. Las vistas de ayuda no se exportan. Errores, cancelación y datos no soportados deben conservar el último estado válido. Estas son condiciones de OPEN-TOON que se concretarán por operación al implementar.

## AUD-001 — Importar sonido

Decodificar formatos admitidos y conservar muestra original y metadatos.

**Aceptación inicial:** Un archivo incompatible genera error sin pista vacía residual.

**Alcance:** `base` · **Nivel:** `core` · **Estado:** `not_started`.

**Referencia:** [H25-230ab35138](https://docs.toonboom.com/help/harmony-25/premium/sound/import-sound.html) · `documentation_linked`.

## AUD-002 — Forma de onda

Representar amplitud a diferentes niveles de zoom temporal.

**Aceptación inicial:** El pico de referencia se alinea con su muestra de audio.

**Alcance:** `base` · **Nivel:** `core` · **Estado:** `not_started`.

**Referencia:** [H25-9949e88183](https://docs.toonboom.com/help/harmony-25/premium/sound/change-sound-display.html) · `documentation_linked`.

## AUD-003 — Recorte y colocación

Mover inicio, ajustar entrada/salida y repetir clips de forma no destructiva.

**Aceptación inicial:** Recortar y deshacer recupera el audio completo.

**Alcance:** `base` · **Nivel:** `core` · **Estado:** `not_started`.

**Referencia:** [H25-dd43ecfd03](https://docs.toonboom.com/help/harmony-25/premium/sound/trim-sound.html) · `documentation_linked`.

## AUD-004 — Mezcla por pistas

Controlar volumen y escuchar varias pistas sincronizadas.

**Aceptación inicial:** Dos pistas alineadas se mezclan sin desplazamiento temporal.

**Alcance:** `base` · **Nivel:** `core` · **Estado:** `not_started`.

**Referencia:** [H25-0e06de0396](https://docs.toonboom.com/help/harmony-25/premium/sound/mix-sound-volume.html) · `documentation_linked`.

## AUD-005 — Scrubbing

Escuchar fragmentos al recorrer frames, incluyendo modo de barrido continuo.

**Aceptación inicial:** Arrastrar al frame marcado reproduce el fragmento esperado.

**Alcance:** `base` · **Nivel:** `pro` · **Estado:** `not_started`.

**Referencia:** [H25-1dda153eb6](https://docs.toonboom.com/help/harmony-25/premium/sound/about-analog-sound-scrub.html) · `documentation_linked`.

## AUD-006 — Detección de lip-sync

Obtener candidatos de fonemas o visemas desde audio con resultados editables.

**Aceptación inicial:** La detección no sobrescribe correcciones manuales sin elección explícita.

**Alcance:** `base` · **Nivel:** `pro` · **Estado:** `not_started`.

**Referencia:** [H25-9bdd7992ce](https://docs.toonboom.com/help/harmony-25/premium/sound/generate-lip-sync-detection.html) · `documentation_linked`.

## AUD-007 — Mapeo a bocas

Asociar etiquetas detectadas a dibujos del personaje.

**Aceptación inicial:** Una etiqueta sin dibujo asignado queda señalada.

**Alcance:** `base` · **Nivel:** `pro` · **Estado:** `not_started`.

**Referencia:** [H25-03023fd15a](https://docs.toonboom.com/help/harmony-25/premium/sound/map-lip-sync-detection.html) · `documentation_linked`.

## AUD-008 — Corrección manual de lip-sync

Editar bocas y duración independientemente del detector.

**Aceptación inicial:** Una corrección manual se conserva al guardar y reabrir.

**Alcance:** `base` · **Nivel:** `core` · **Estado:** `not_started`.

**Referencia:** [H25-821cc5e71d](https://docs.toonboom.com/help/harmony-25/premium/sound/manual-lip-sync.html) · `documentation_linked`.

## AUD-009 — Biblioteca de audio

Localizar, preescuchar y reutilizar audio de escena sin duplicarlo innecesariamente.

**Aceptación inicial:** Renombrar una entrada de biblioteca no mueve el archivo fuente.

**Alcance:** `base` · **Nivel:** `pro` · **Estado:** `not_started`.

**Referencia:** [H25-75d79a7b9b](https://docs.toonboom.com/help/harmony-25/premium/release-notes/harmony/harmony-25-2-release-notes.html) · `documentation_linked`.

## AUD-010 — Exportar mezcla

Exportar audio por rango y sincronizarlo con el render.

**Aceptación inicial:** La duración exportada coincide con el rango racional de escena.

**Alcance:** `base` · **Nivel:** `pro` · **Estado:** `not_started`.

**Referencia:** [H25-150dae0098](https://docs.toonboom.com/help/harmony-25/premium/sound/export-sound.html) · `documentation_linked`.
