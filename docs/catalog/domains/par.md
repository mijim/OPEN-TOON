# PAR — Partículas

[Volver al catálogo](../README.md)

> Vista generada desde `features.json` y `domains.json`; no editar a mano.

**Flujo:** Emitir → aplicar fuerzas → representar → hornear.

**Módulo:** `particles`.

**Entidades:** ParticleSystem, Emitter, ParticleState, Force, SimulationCache.

**Relaciones:** NOD, ANI.

**Riesgo principal:** La simulación debe ser reproducible y permitir scrubbing.

## Contrato común

Las mutaciones deben respetar transacciones, undo/redo y persistencia. Las vistas de ayuda no se exportan. Errores, cancelación y datos no soportados deben conservar el último estado válido. Estas son condiciones de OPEN-TOON que se concretarán por operación al implementar.

## PAR-001 — Emisores de sprites

Emitir imágenes con tasa, vida y semilla controladas.

**Aceptación inicial:** La misma semilla produce la misma secuencia.

**Alcance:** `base` · **Nivel:** `advanced` · **Estado:** `not_started`.

**Evidencia:** `proposal`.

## PAR-002 — Regiones de emisión

Definir emisión por regiones planas o espaciales.

**Aceptación inicial:** Las partículas nacen únicamente dentro de la región elegida.

**Alcance:** `base` · **Nivel:** `advanced` · **Estado:** `not_started`.

**Evidencia:** `proposal`.

## PAR-003 — Velocidad y variación

Asignar velocidad, tamaño, orientación y dispersión por parámetros.

**Aceptación inicial:** Con variación cero todas las partículas comparten el valor inicial.

**Alcance:** `base` · **Nivel:** `advanced` · **Estado:** `not_started`.

**Evidencia:** `proposal`.

## PAR-004 — Fuerzas y movimiento

Aplicar gravedad, viento, fricción, vórtices y atracción o repulsión.

**Aceptación inicial:** Con todas las fuerzas a cero se conserva la velocidad inicial.

**Alcance:** `base` · **Nivel:** `advanced` · **Estado:** `not_started`.

**Evidencia:** `proposal`.

## PAR-005 — Colisiones y vida

Gestionar rebote, muerte y regiones sumidero.

**Aceptación inicial:** Una partícula expirada deja de contribuir al render.

**Alcance:** `base` · **Nivel:** `advanced` · **Estado:** `not_started`.

**Evidencia:** `proposal`.

## PAR-006 — Bake y composición

Cachear simulaciones y mezclar sistemas con otras capas.

**Aceptación inicial:** Saltar a un frame horneado reproduce el resultado secuencial.

**Alcance:** `base` · **Nivel:** `advanced` · **Estado:** `not_started`.

**Evidencia:** `proposal`.

## PAR-007 — Plantillas de partículas

Guardar sistemas reutilizables con sus assets y parámetros.

**Aceptación inicial:** Importar una plantilla conserva semilla y aspecto.

**Alcance:** `base` · **Nivel:** `advanced` · **Estado:** `not_started`.

**Evidencia:** `proposal`.
