# DEF — Deformadores y mallas

[Volver al catálogo](../README.md)

> Vista generada desde `features.json` y `domains.json`; no editar a mano.

**Flujo:** Preparar reposo → definir influencias → animar → evaluar → hornear.

**Módulo:** `deformation`.

**Entidades:** DeformationChain, RestPose, Influence, Mesh, WeightMap.

**Relaciones:** RIG.

**Riesgo principal:** Calidad de deformación, auto-intersecciones y texturas complejas.

## Contrato común

Las mutaciones deben respetar transacciones, undo/redo y persistencia. Las vistas de ayuda no se exportan. Errores, cancelación y datos no soportados deben conservar el último estado válido. Estas son condiciones de OPEN-TOON que se concretarán por operación al implementar.

## DEF-001 — Deformación por huesos

Articular un dibujo mediante una cadena con juntas e influencias.

**Aceptación inicial:** Una extremidad se flexiona manteniendo conexión entre segmentos.

**Alcance:** `base` · **Nivel:** `pro` · **Estado:** `not_started`.

**Evidencia:** `proposal`.

## DEF-002 — Game bones

Representar esqueletos orientados a exportación de runtime con límites documentados.

**Aceptación inicial:** La exportación conserva jerarquía y transformaciones de reposo.

**Alcance:** `base` · **Nivel:** `advanced` · **Estado:** `not_started`.

**Evidencia:** `proposal`.

## DEF-003 — Curvas de deformación

Controlar la forma mediante segmentos curvos y tangentes.

**Aceptación inicial:** Mover una tangente produce una transición continua.

**Alcance:** `base` · **Nivel:** `pro` · **Estado:** `not_started`.

**Evidencia:** `proposal`.

## DEF-004 — Envelope

Deformar la silueta mediante una envolvente cerrada.

**Aceptación inicial:** Cerrar la envolvente no introduce una discontinuidad en su unión.

**Alcance:** `base` · **Nivel:** `pro` · **Estado:** `not_started`.

**Evidencia:** `proposal`.

## DEF-005 — Free Form

Deformar regiones interiores mediante malla y controles internos.

**Aceptación inicial:** Una textura cuadriculada sigue la deformación del interior.

**Alcance:** `base` · **Nivel:** `advanced` · **Estado:** `not_started`.

**Evidencia:** `proposal`.

## DEF-006 — Shape-aware

Adaptar pesos a la forma usando controles de punto, hueso y jaula.

**Aceptación inicial:** Mover un control localizado reduce influencia fuera de su región.

**Alcance:** `base` · **Nivel:** `advanced` · **Estado:** `not_started`.

**Evidencia:** `proposal`.

## DEF-007 — Weighted deform

Combinar influencias de curvas, puntos y pegs en un campo de deformación.

**Aceptación inicial:** Dos fuentes solapadas se mezclan sin salto brusco en la frontera.

**Alcance:** `base` · **Nivel:** `advanced` · **Estado:** `not_started`.

**Evidencia:** `proposal`.

## DEF-008 — Edición de pose de reposo

Separar bind/rest pose del estado animado y actualizarlo de forma explícita.

**Aceptación inicial:** Restablecer lleva a la pose de reposo sin borrar las claves.

**Alcance:** `base` · **Nivel:** `pro` · **Estado:** `not_started`.

**Evidencia:** `proposal`.

## DEF-009 — Regiones de influencia

Configurar áreas elípticas o de forma y radios de transición.

**Aceptación inicial:** Reducir un radio limita el área deformada sin cambiar el control.

**Alcance:** `base` · **Nivel:** `advanced` · **Estado:** `not_started`.

**Evidencia:** `proposal`.

## DEF-010 — Rigs con múltiples poses

Asignar cadenas de deformación a sustituciones compatibles.

**Aceptación inicial:** Cambiar dibujo activa la cadena correcta sin referencias huérfanas.

**Alcance:** `base` · **Nivel:** `advanced` · **Estado:** `not_started`.

**Evidencia:** `proposal`.

## DEF-011 — Kinematic output

Adjuntar otros elementos al resultado de un deformador.

**Aceptación inicial:** Un accesorio sigue el extremo del brazo deformado.

**Alcance:** `base` · **Nivel:** `advanced` · **Estado:** `not_started`.

**Evidencia:** `proposal`.

## DEF-012 — Point kinematic output

Muestrear posiciones de una curva deformada para controlar otros objetos.

**Aceptación inicial:** El objeto adjunto sigue el punto elegido durante toda la animación.

**Alcance:** `base` · **Nivel:** `advanced` · **Estado:** `not_started`.

**Evidencia:** `proposal`.

## DEF-013 — Conversión a dibujos

Hornear deformaciones evaluadas como dibujos editables.

**Aceptación inicial:** El dibujo horneado reproduce la pose y no depende del rig original.

**Alcance:** `base` · **Nivel:** `pro` · **Estado:** `not_started`.

**Evidencia:** `proposal`.

## DEF-014 — Generación de envolventes

Derivar controles iniciales de la geometría del dibujo con limpieza posterior.

**Aceptación inicial:** La envolvente propuesta puede editarse antes de confirmarla.

**Alcance:** `base` · **Nivel:** `advanced` · **Estado:** `not_started`.

**Evidencia:** `proposal`.

## DEF-015 — Visibilidad y calidad de deformación

Activar controles y niveles de preview sin alterar resultado final.

**Aceptación inicial:** Ocultar controles no desactiva el deformador.

**Alcance:** `base` · **Nivel:** `pro` · **Estado:** `not_started`.

**Evidencia:** `proposal`.
