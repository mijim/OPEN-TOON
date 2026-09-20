# ANI — Transformaciones, curvas y animación por claves

[Volver al catálogo](../README.md)

> Vista generada desde `features.json` y `domains.json`; no editar a mano.

**Flujo:** Colocar claves → interpolar → ajustar curvas → reutilizar movimiento.

**Módulo:** `animation-curves`.

**Entidades:** AnimatableProperty, Keyframe, Curve, Transform, Expression.

**Relaciones:** TIM, LYR.

**Riesgo principal:** Orden de matrices, pivotes y evaluación de curvas.

## Contrato común

Las mutaciones deben respetar transacciones, undo/redo y persistencia. Las vistas de ayuda no se exportan. Errores, cancelación y datos no soportados deben conservar el último estado válido. Estas son condiciones de OPEN-TOON que se concretarán por operación al implementar.

## ANI-001 — Transformaciones de capa

Animar traslación, rotación, escala, sesgo y opacidad con unidades coherentes.

**Aceptación inicial:** Una escala negativa no produce valores indefinidos en la interpolación.

**Alcance:** `base` · **Nivel:** `core` · **Estado:** `not_started`.

**Evidencia:** `proposal`.

## ANI-002 — Modo setup y animate

Separar cambios de reposo de inserción o modificación de claves.

**Aceptación inicial:** Mover un objeto en modo setup no añade claves.

**Alcance:** `base` · **Nivel:** `core` · **Estado:** `not_started`.

**Evidencia:** `proposal`.

## ANI-003 — Pegs

Usar nodos de transformación independientes del contenido gráfico.

**Aceptación inicial:** Mover el peg transforma todos sus hijos sin alterar sus dibujos.

**Alcance:** `base` · **Nivel:** `core` · **Estado:** `not_started`.

**Evidencia:** `proposal`.

## ANI-004 — Claves interpoladas y sostenidas

Soportar segmentos continuos y saltos de pose controlados.

**Aceptación inicial:** Un segmento stepped mantiene la pose hasta el siguiente keyframe.

**Alcance:** `base` · **Nivel:** `core` · **Estado:** `not_started`.

**Evidencia:** `proposal`.

## ANI-005 — Autokey y edición de claves

Crear claves por propiedad o conjunto sin claves accidentales.

**Aceptación inicial:** Con autokey apagado se distingue edición local de edición animada.

**Alcance:** `base` · **Nivel:** `core` · **Estado:** `not_started`.

**Evidencia:** `proposal`.

## ANI-006 — Editor de funciones

Editar curvas con tangentes, rangos, valores y unidades visibles.

**Aceptación inicial:** Una tangente modificada produce el mismo valor en preview y exportación.

**Alcance:** `base` · **Nivel:** `core` · **Estado:** `not_started`.

**Evidencia:** `proposal`.

## ANI-007 — Ease y velocidad

Controlar aceleración y desaceleración por curvas y presets.

**Aceptación inicial:** Un ease no mueve las posiciones finales de las claves.

**Alcance:** `base` · **Nivel:** `core` · **Estado:** `not_started`.

**Evidencia:** `proposal`.

## ANI-008 — Trayectorias y velocidad espacial

Distinguir forma del recorrido y avance temporal por él.

**Aceptación inicial:** Cambiar velocidad mantiene la geometría de la trayectoria.

**Alcance:** `base` · **Nivel:** `pro` · **Estado:** `not_started`.

**Evidencia:** `proposal`.

## ANI-009 — Edición de varias claves

Desplazar, escalar y copiar bloques de claves entre pistas compatibles.

**Aceptación inicial:** Escalar un bloque conserva el orden o explica colisiones temporales.

**Alcance:** `base` · **Nivel:** `pro` · **Estado:** `not_started`.

**Evidencia:** `proposal`.

## ANI-010 — Copiar movimiento

Transferir animación entre objetos con reglas sobre pivotes y unidades.

**Aceptación inicial:** Copiar movimiento no duplica el dibujo subyacente.

**Alcance:** `base` · **Nivel:** `pro` · **Estado:** `not_started`.

**Evidencia:** `proposal`.

## ANI-011 — Expresiones

Evaluar relaciones entre atributos con detección de ciclos y errores visibles.

**Aceptación inicial:** Una dependencia circular no bloquea la aplicación.

**Alcance:** `base` · **Nivel:** `advanced` · **Estado:** `not_started`.

**Evidencia:** `proposal`.

## ANI-012 — Captura de movimiento manual

Registrar una trayectoria gestual como datos temporales editables.

**Aceptación inicial:** La trayectoria capturada puede simplificarse y deshacerse.

**Alcance:** `base` · **Nivel:** `pro` · **Estado:** `not_started`.

**Evidencia:** `proposal`.

## ANI-013 — Editor numérico animable

Editar valores y navegar claves directamente desde el inspector.

**Aceptación inicial:** El inspector muestra si el valor pertenece a una clave o es interpolado.

**Alcance:** `base` · **Nivel:** `pro` · **Estado:** `not_started`.

**Evidencia:** `proposal`.
