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

**Evidencia:** `proposal`.

## AUD-002 — Forma de onda

Representar amplitud a diferentes niveles de zoom temporal.

**Aceptación inicial:** El pico de referencia se alinea con su muestra de audio.

**Alcance:** `base` · **Nivel:** `core` · **Estado:** `not_started`.

**Evidencia:** `proposal`.

## AUD-003 — Recorte y colocación

Mover inicio, ajustar entrada/salida y repetir clips de forma no destructiva.

**Aceptación inicial:** Recortar y deshacer recupera el audio completo.

**Alcance:** `base` · **Nivel:** `core` · **Estado:** `not_started`.

**Evidencia:** `proposal`.

## AUD-004 — Mezcla por pistas

Controlar volumen y escuchar varias pistas sincronizadas.

**Aceptación inicial:** Dos pistas alineadas se mezclan sin desplazamiento temporal.

**Alcance:** `base` · **Nivel:** `core` · **Estado:** `not_started`.

**Evidencia:** `proposal`.

## AUD-005 — Scrubbing

Escuchar fragmentos al recorrer frames, incluyendo modo de barrido continuo.

**Aceptación inicial:** Arrastrar al frame marcado reproduce el fragmento esperado.

**Alcance:** `base` · **Nivel:** `pro` · **Estado:** `not_started`.

**Evidencia:** `proposal`.

## AUD-006 — Detección de lip-sync

Obtener candidatos de fonemas o visemas desde audio con resultados editables.

**Aceptación inicial:** La detección no sobrescribe correcciones manuales sin elección explícita.

**Alcance:** `base` · **Nivel:** `pro` · **Estado:** `not_started`.

**Evidencia:** `proposal`.

## AUD-007 — Mapeo a bocas

Asociar etiquetas detectadas a dibujos del personaje.

**Aceptación inicial:** Una etiqueta sin dibujo asignado queda señalada.

**Alcance:** `base` · **Nivel:** `pro` · **Estado:** `not_started`.

**Evidencia:** `proposal`.

## AUD-008 — Corrección manual de lip-sync

Editar bocas y duración independientemente del detector.

**Aceptación inicial:** Una corrección manual se conserva al guardar y reabrir.

**Alcance:** `base` · **Nivel:** `core` · **Estado:** `not_started`.

**Evidencia:** `proposal`.

## AUD-009 — Biblioteca de audio

Localizar, preescuchar y reutilizar audio de escena sin duplicarlo innecesariamente.

**Aceptación inicial:** Renombrar una entrada de biblioteca no mueve el archivo fuente.

**Alcance:** `base` · **Nivel:** `pro` · **Estado:** `not_started`.

**Evidencia:** `proposal`.

## AUD-010 — Exportar mezcla

Exportar audio por rango y sincronizarlo con el render.

**Aceptación inicial:** La duración exportada coincide con el rango racional de escena.

**Alcance:** `base` · **Nivel:** `pro` · **Estado:** `not_started`.

**Evidencia:** `proposal`.
