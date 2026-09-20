# RIG — Rigging, jerarquías e inversa cinemática

[Volver al catálogo](../README.md)

> Vista generada desde `features.json` y `domains.json`; no editar a mano.

**Flujo:** Despiece → pivotes → jerarquía → controles → poses.

**Módulo:** `rigging`.

**Entidades:** Rig, Joint, PegHierarchy, Constraint, DrawingSubstitution.

**Relaciones:** ANI, VEC.

**Riesgo principal:** Ciclos de dependencia y saltos de pose al modificar jerarquías.

## Contrato común

Las mutaciones deben respetar transacciones, undo/redo y persistencia. Las vistas de ayuda no se exportan. Errores, cancelación y datos no soportados deben conservar el último estado válido. Estas son condiciones de OPEN-TOON que se concretarán por operación al implementar.

## RIG-001 — Despiece de personaje

Separar partes del modelo preservando registro y paletas.

**Aceptación inicial:** Las piezas reconstruyen visualmente el personaje original en reposo.

**Alcance:** `base` · **Nivel:** `pro` · **Estado:** `not_started`.

**Evidencia:** `proposal`.

## RIG-002 — Jerarquías de transformación

Construir cadenas padre-hijo con dibujo y pegs separados.

**Aceptación inicial:** Reparentar con conservar mundo no desplaza el personaje.

**Alcance:** `base` · **Nivel:** `core` · **Estado:** `not_started`.

**Evidencia:** `proposal`.

## RIG-003 — Pivotes permanentes y temporales

Editar centro de rotación con distinción de alcance.

**Aceptación inicial:** Mover el pivote temporal no modifica el pivote guardado del rig.

**Alcance:** `base` · **Nivel:** `core` · **Estado:** `not_started`.

**Evidencia:** `proposal`.

## RIG-004 — Orden Z y ajustes de profundidad

Organizar superposición de piezas con control fino.

**Aceptación inicial:** El brazo puede pasar delante y detrás del torso con resultado estable.

**Alcance:** `base` · **Nivel:** `pro` · **Estado:** `not_started`.

**Evidencia:** `proposal`.

## RIG-005 — Articulaciones y Auto Patch

Resolver juntas y ocultación de líneas entre piezas superpuestas.

**Aceptación inicial:** La junta de prueba no muestra una costura al flexionar el brazo.

**Alcance:** `base` · **Nivel:** `pro` · **Estado:** `not_started`.

**Evidencia:** `proposal`.

## RIG-006 — Sustitución de dibujos

Cambiar bocas, manos o vistas de una pieza manteniendo la animación.

**Aceptación inicial:** Cambiar una boca no altera la transformación de cabeza.

**Alcance:** `base` · **Nivel:** `core` · **Estado:** `not_started`.

**Evidencia:** `proposal`.

## RIG-007 — Sustitución múltiple

Cambiar variantes coordinadas en varios elementos del personaje.

**Aceptación inicial:** Cambiar vista sustituye las piezas asignadas en una única transacción.

**Alcance:** `base` · **Nivel:** `pro` · **Estado:** `not_started`.

**Evidencia:** `proposal`.

## RIG-008 — Navegación de jerarquía

Seleccionar padre, hijo o cadena sin buscar manualmente cada nodo.

**Aceptación inicial:** La navegación respeta el grupo y no cruza a otro personaje.

**Alcance:** `base` · **Nivel:** `pro` · **Estado:** `not_started`.

**Evidencia:** `proposal`.

## RIG-009 — Inversa cinemática

Resolver articulaciones desde un objetivo con límites configurables.

**Aceptación inicial:** Arrastrar una mano mantiene longitudes cuando el modo exige rigidez.

**Alcance:** `base` · **Nivel:** `advanced` · **Estado:** `not_started`.

**Evidencia:** `proposal`.

## RIG-010 — Nails y restricciones IK

Fijar puntos durante manipulación de otras partes.

**Aceptación inicial:** Un pie fijado permanece en posición mientras se desplaza el torso.

**Alcance:** `base` · **Nivel:** `advanced` · **Estado:** `not_started`.

**Evidencia:** `proposal`.

## RIG-011 — Claves de restricciones

Animar el estado de restricciones y transiciones de control.

**Aceptación inicial:** Activar una fijación en un frame no cambia frames anteriores.

**Alcance:** `base` · **Nivel:** `advanced` · **Estado:** `not_started`.

**Evidencia:** `proposal`.

## RIG-012 — Copiar poses

Guardar y transferir poses con mapeo estable de piezas.

**Aceptación inicial:** Aplicar una pose no sobrescribe propiedades fuera de su conjunto.

**Alcance:** `base` · **Nivel:** `pro` · **Estado:** `not_started`.

**Evidencia:** `proposal`.

## RIG-013 — Breakdown Assistant

Mezclar valores entre poses vecinas por porcentaje y selección de atributos.

**Aceptación inicial:** Los extremos cero y cien reproducen las poses de referencia.

**Alcance:** `base` · **Nivel:** `pro` · **Estado:** `not_started`.

**Evidencia:** `proposal`.

## RIG-014 — Guías y convenciones de rig

Identificar piezas de control, guías y nombres sin incluirlas en render.

**Aceptación inicial:** Una guía visible en setup queda excluida de salida final.

**Alcance:** `base` · **Nivel:** `pro` · **Estado:** `not_started`.

**Evidencia:** `proposal`.
