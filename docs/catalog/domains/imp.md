# IMP — Importación, escaneo e intercambio

[Volver al catálogo](../README.md)

> Vista generada desde `features.json` y `domains.json`; no editar a mano.

**Flujo:** Importar → verificar conversiones → enlazar recursos → corregir pérdidas.

**Módulo:** `interchange`.

**Entidades:** ImportJob, FormatAdapter, ConversionReport, ExternalAsset.

**Relaciones:** PRJ, LYR.

**Riesgo principal:** La extensión de archivo no implica fidelidad de todas sus funciones.

## Contrato común

Las mutaciones deben respetar transacciones, undo/redo y persistencia. Las vistas de ayuda no se exportan. Errores, cancelación y datos no soportados deben conservar el último estado válido. Estas son condiciones de OPEN-TOON que se concretarán por operación al implementar.

## IMP-001 — Imágenes y secuencias

Importar imágenes sueltas o secuencias con alfa, orden y exposición explícitos.

**Aceptación inicial:** Una secuencia con huecos informa cómo los resuelve.

**Alcance:** `base` · **Nivel:** `core` · **Estado:** `not_started`.

**Evidencia:** `proposal`.

## IMP-002 — PSD por capas

Leer jerarquía, opacidad y modos soportados, informando de pérdidas.

**Aceptación inicial:** Un modo no soportado se informa antes de aceptar la conversión.

**Alcance:** `base` · **Nivel:** `pro` · **Estado:** `not_started`.

**Evidencia:** `proposal`.

## IMP-003 — Layout PSD

Reutilizar composición y posiciones de un layout con escala conocida.

**Aceptación inicial:** Las capas conservan registro relativo tras importar.

**Alcance:** `base` · **Nivel:** `pro` · **Estado:** `not_started`.

**Evidencia:** `proposal`.

## IMP-004 — Vectores externos

Convertir formatos vectoriales compatibles preservando curvas, rellenos y transformaciones soportadas.

**Aceptación inicial:** El informe enumera filtros o fuentes no trasladables.

**Alcance:** `base` · **Nivel:** `pro` · **Estado:** `not_started`.

**Evidencia:** `proposal`.

## IMP-005 — Vídeo de referencia

Importar vídeo para referencia o extracción de frames con reloj explícito.

**Aceptación inicial:** Un clip de tasa variable se normaliza sin deriva oculta.

**Alcance:** `base` · **Nivel:** `pro` · **Estado:** `not_started`.

**Evidencia:** `proposal`.

## IMP-006 — Vincular imagen externa

Conservar enlace y permitir actualizar contenido desde disco.

**Aceptación inicial:** Una actualización no borra la transformación de la capa.

**Alcance:** `base` · **Nivel:** `pro` · **Estado:** `not_started`.

**Evidencia:** `proposal`.

## IMP-007 — Escaneo

Adquirir dibujos con resolución, registro y orden de numeración.

**Aceptación inicial:** Reescanear una hoja puede sustituir su dibujo sin cambiar exposiciones.

**Alcance:** `base` · **Nivel:** `advanced` · **Estado:** `not_started`.

**Evidencia:** `proposal`.

## IMP-008 — Vectorización

Convertir escaneos a trazos o regiones con parámetros repetibles.

**Aceptación inicial:** El mismo preset y entrada producen igual salida.

**Alcance:** `base` · **Nivel:** `advanced` · **Estado:** `not_started`.

**Evidencia:** `proposal`.

## IMP-009 — FLA y SWF heredados

Documentar conversión asistida y límites de formatos antiguos.

**Aceptación inicial:** Una función no convertible queda en un informe, no desaparece sin aviso.

**Alcance:** `legacy_candidate` · **Nivel:** `legacy` · **Estado:** `not_started`.

**Evidencia:** `proposal`.

## IMP-010 — Intercambio editorial

Recibir layouts o animatics como escenas con timing y recursos.

**Aceptación inicial:** La duración del plano importado coincide con el animatic de referencia.

**Alcance:** `base` · **Nivel:** `pro` · **Estado:** `not_started`.

**Evidencia:** `proposal`.
