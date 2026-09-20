# MOR — Morphing vectorial

[Volver al catálogo](../README.md)

> Vista generada desde `features.json` y `domains.json`; no editar a mano.

**Flujo:** Elegir extremos → establecer correspondencias → interpolar → corregir.

**Módulo:** `morphing`.

**Entidades:** MorphSequence, CorrespondenceHint, MorphKey.

**Relaciones:** VEC, TIM.

**Riesgo principal:** Topologías incompatibles, huecos y trazos degenerados.

## Contrato común

Las mutaciones deben respetar transacciones, undo/redo y persistencia. Las vistas de ayuda no se exportan. Errores, cancelación y datos no soportados deben conservar el último estado válido. Estas son condiciones de OPEN-TOON que se concretarán por operación al implementar.

## MOR-001 — Interpolación de dibujos

Generar intercalados entre dibujos vectoriales compatibles.

**Aceptación inicial:** El primer y último frame son idénticos a los extremos.

**Alcance:** `base` · **Nivel:** `advanced` · **Estado:** `not_started`.

**Evidencia:** `proposal`.

## MOR-002 — Reglas de compatibilidad

Diagnosticar incompatibilidades de regiones y colores antes de interpolar.

**Aceptación inicial:** Una entrada bitmap se rechaza en el modo vectorial.

**Alcance:** `base` · **Nivel:** `advanced` · **Estado:** `not_started`.

**Evidencia:** `proposal`.

## MOR-003 — Hints de correspondencia

Permitir guiar qué puntos, contornos o líneas se corresponden.

**Aceptación inicial:** Añadir una correspondencia modifica el segmento esperado.

**Alcance:** `base` · **Nivel:** `advanced` · **Estado:** `not_started`.

**Evidencia:** `proposal`.

## MOR-004 — Claves de morphing intermedias

Convertir un resultado intermedio en dibujo clave para añadir detalles.

**Aceptación inicial:** La nueva clave divide el morph y sigue siendo editable.

**Alcance:** `base` · **Nivel:** `advanced` · **Estado:** `not_started`.

**Evidencia:** `proposal`.

## MOR-005 — Timing y easing del morph

Separar progreso temporal del emparejamiento geométrico.

**Aceptación inicial:** Ajustar ease conserva dibujos extremos y duración.

**Alcance:** `base` · **Nivel:** `advanced` · **Estado:** `not_started`.

**Evidencia:** `proposal`.

## MOR-006 — Huecos y transparencia

Resolver regiones interiores y alfa con reglas visibles.

**Aceptación inicial:** Un anillo conserva el hueco en toda la secuencia válida.

**Alcance:** `base` · **Nivel:** `advanced` · **Estado:** `not_started`.

**Evidencia:** `proposal`.
