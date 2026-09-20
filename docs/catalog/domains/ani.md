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

**Referencia:** [H25-20379c4ed5](https://docs.toonboom.com/help/harmony-25/premium/staging/about-transform-tool.html) · `documentation_linked`.

## ANI-002 — Modo setup y animate

Separar cambios de reposo de inserción o modificación de claves.

**Aceptación inicial:** Mover un objeto en modo setup no añade claves.

**Alcance:** `base` · **Nivel:** `core` · **Estado:** `not_started`.

**Referencia:** [H25-0ef435e1bc](https://docs.toonboom.com/help/harmony-25/premium/staging/set-animate-off-mode.html) · `documentation_linked`.

## ANI-003 — Pegs

Usar nodos de transformación independientes del contenido gráfico.

**Aceptación inicial:** Mover el peg transforma todos sus hijos sin alterar sus dibujos.

**Alcance:** `base` · **Nivel:** `core` · **Estado:** `not_started`.

**Referencia:** [H25-aefd91e26a](https://docs.toonboom.com/help/harmony-25/premium/motion-path/about-peg.html) · `documentation_linked`.

## ANI-004 — Claves interpoladas y sostenidas

Soportar segmentos continuos y saltos de pose controlados.

**Aceptación inicial:** Un segmento stepped mantiene la pose hasta el siguiente keyframe.

**Alcance:** `base` · **Nivel:** `core` · **Estado:** `not_started`.

**Referencia:** [H25-43d0d2315c](https://docs.toonboom.com/help/harmony-25/premium/motion-path/switch-keyframe-type.html) · `documentation_linked`.

## ANI-005 — Autokey y edición de claves

Crear claves por propiedad o conjunto sin claves accidentales.

**Aceptación inicial:** Con autokey apagado se distingue edición local de edición animada.

**Alcance:** `base` · **Nivel:** `core` · **Estado:** `not_started`.

**Referencia:** [H25-af0996665f](https://docs.toonboom.com/help/harmony-25/premium/motion-path/about-keyframe-creation.html) · `documentation_linked`.

## ANI-006 — Editor de funciones

Editar curvas con tangentes, rangos, valores y unidades visibles.

**Aceptación inicial:** Una tangente modificada produce el mismo valor en preview y exportación.

**Alcance:** `base` · **Nivel:** `core` · **Estado:** `not_started`.

**Referencia:** [H25-2e02d6d25d](https://docs.toonboom.com/help/harmony-25/premium/motion-path/about-function.html) · `documentation_linked`.

## ANI-007 — Ease y velocidad

Controlar aceleración y desaceleración por curvas y presets.

**Aceptación inicial:** Un ease no mueve las posiciones finales de las claves.

**Alcance:** `base` · **Nivel:** `core` · **Estado:** `not_started`.

**Referencia:** [H25-74363b3536](https://docs.toonboom.com/help/harmony-25/premium/cut-out-animation/about-ease.html) · `documentation_linked`.

## ANI-008 — Trayectorias y velocidad espacial

Distinguir forma del recorrido y avance temporal por él.

**Aceptación inicial:** Cambiar velocidad mantiene la geometría de la trayectoria.

**Alcance:** `base` · **Nivel:** `pro` · **Estado:** `not_started`.

**Referencia:** [H25-cf6ae6b12e](https://docs.toonboom.com/help/harmony-25/premium/motion-path/about-velocity.html) · `documentation_linked`.

## ANI-009 — Edición de varias claves

Desplazar, escalar y copiar bloques de claves entre pistas compatibles.

**Aceptación inicial:** Escalar un bloque conserva el orden o explica colisiones temporales.

**Alcance:** `base` · **Nivel:** `pro` · **Estado:** `not_started`.

**Referencia:** [H25-c31d38a2e7](https://docs.toonboom.com/help/harmony-25/premium/motion-path/animate-several-keyframes.html) · `documentation_linked`.

## ANI-010 — Copiar movimiento

Transferir animación entre objetos con reglas sobre pivotes y unidades.

**Aceptación inicial:** Copiar movimiento no duplica el dibujo subyacente.

**Alcance:** `base` · **Nivel:** `pro` · **Estado:** `not_started`.

**Referencia:** [H25-a5ec1bae31](https://docs.toonboom.com/help/harmony-25/premium/motion-path/about-copy-motion.html) · `documentation_linked`.

## ANI-011 — Expresiones

Evaluar relaciones entre atributos con detección de ciclos y errores visibles.

**Aceptación inicial:** Una dependencia circular no bloquea la aplicación.

**Alcance:** `base` · **Nivel:** `advanced` · **Estado:** `not_started`.

**Referencia:** [H25-fcfc1dd9fc](https://docs.toonboom.com/help/harmony-25/premium/motion-path/about-expression-column.html) · `documentation_linked`.

## ANI-012 — Captura de movimiento manual

Registrar una trayectoria gestual como datos temporales editables.

**Aceptación inicial:** La trayectoria capturada puede simplificarse y deshacerse.

**Alcance:** `base` · **Nivel:** `pro` · **Estado:** `not_started`.

**Referencia:** [H25-457e6e19ce](https://docs.toonboom.com/help/harmony-25/premium/staging/use-capture-motion-tool.html) · `documentation_linked`.

## ANI-013 — Editor numérico animable

Editar valores y navegar claves directamente desde el inspector.

**Aceptación inicial:** El inspector muestra si el valor pertenece a una clave o es interpolado.

**Alcance:** `base` · **Nivel:** `pro` · **Estado:** `not_started`.

**Referencia:** [H25-75d79a7b9b](https://docs.toonboom.com/help/harmony-25/premium/release-notes/harmony/harmony-25-2-release-notes.html) · `documentation_linked`.
