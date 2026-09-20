# LYR — Capas, dibujos y subcapas artísticas

[Volver al catálogo](../README.md)

> Vista generada desde `features.json` y `domains.json`; no editar a mano.

**Flujo:** Crear capa → crear dibujo → exponerlo → reutilizar o duplicar.

**Módulo:** `document`.

**Entidades:** Layer, Drawing, DrawingId, ArtLayer, ExposureTrack.

**Relaciones:** PRJ.

**Riesgo principal:** Confundir exposición, dibujo compartido y copia independiente.

## Contrato común

Las mutaciones deben respetar transacciones, undo/redo y persistencia. Las vistas de ayuda no se exportan. Errores, cancelación y datos no soportados deben conservar el último estado válido. Estas son condiciones de OPEN-TOON que se concretarán por operación al implementar.

## LYR-001 — Capas tipadas

Crear capas de dibujo, sonido y transformación con atributos específicos.

**Aceptación inicial:** El inspector impide asignar muestras de audio a una capa vectorial.

**Alcance:** `base` · **Nivel:** `core` · **Estado:** `not_started`.

**Referencia:** [H25-941df44055](https://docs.toonboom.com/help/harmony-25/premium/layers/layer-column-type.html) · `documentation_linked`.

## LYR-002 — Reordenar y renombrar

Cambiar orden y etiquetas manteniendo identificadores estables.

**Aceptación inicial:** Renombrar una capa no rompe nodos ni curvas vinculadas.

**Alcance:** `base` · **Nivel:** `core` · **Estado:** `not_started`.

**Referencia:** [H25-ceff40db57](https://docs.toonboom.com/help/harmony-25/premium/layers/reorder-layer-column.html) · `documentation_linked`.

## LYR-003 — Visibilidad, bloqueo y solo

Separar visibilidad de edición, bloqueo y aislamiento temporal.

**Aceptación inicial:** Solo se revierte sin perder el estado original de las demás capas.

**Alcance:** `base` · **Nivel:** `core` · **Estado:** `not_started`.

**Referencia:** [H25-7291b58452](https://docs.toonboom.com/help/harmony-25/premium/layers/enable-disable-solo-mode.html) · `documentation_linked`.

## LYR-004 — Grupos de capas

Agrupar y desagrupar conservando las relaciones que afectan al resultado.

**Aceptación inicial:** Desagrupar no altera la posición global de los dibujos.

**Alcance:** `base` · **Nivel:** `core` · **Estado:** `not_started`.

**Referencia:** [H25-06d100d8bb](https://docs.toonboom.com/help/harmony-25/premium/layers/about-group-timeline.html) · `documentation_linked`.

## LYR-005 — Duplicar o clonar

Distinguir copia profunda de instancia que comparte dibujos o funciones.

**Aceptación inicial:** Editar un clon actualiza sus instancias; editar una copia no.

**Alcance:** `base` · **Nivel:** `core` · **Estado:** `not_started`.

**Referencia:** [H25-0b3992eaf8](https://docs.toonboom.com/help/harmony-25/premium/layers/about-clone-duplicate-layer.html) · `documentation_linked`.

## LYR-006 — Sincronización de exposiciones

Permitir pistas vinculadas que compartan la secuencia de dibujos.

**Aceptación inicial:** Cambiar una exposición sincronizada actualiza las pistas asociadas.

**Alcance:** `base` · **Nivel:** `pro` · **Estado:** `not_started`.

**Referencia:** [H25-e3df7474cd](https://docs.toonboom.com/help/harmony-25/premium/layers/about-sync-layer.html) · `documentation_linked`.

## LYR-007 — Gestión de elementos

Identificar dibujos usados y no expuestos; renumerar sin perder referencias.

**Aceptación inicial:** Un dibujo no expuesto no se elimina al vaciar una celda.

**Alcance:** `base` · **Nivel:** `pro` · **Estado:** `not_started`.

**Referencia:** [H25-fd47fa27eb](https://docs.toonboom.com/help/harmony-25/premium/layers/use-element-manager.html) · `documentation_linked`.

## LYR-008 — Line, Colour, Overlay y Underlay

Guardar cuatro subcapas artísticas por dibujo con orden definido.

**Aceptación inicial:** Pintar Colour Art mantiene intacta la línea de Line Art.

**Alcance:** `base` · **Nivel:** `pro` · **Estado:** `not_started`.

**Referencia:** [H25-890432ee4e](https://docs.toonboom.com/help/harmony-25/premium/paperless-animation/about-art-layer.html) · `documentation_linked`.

## LYR-009 — Gestor de art layers

Mover, intercambiar, duplicar y consolidar subcapas en rangos de dibujos.

**Aceptación inicial:** Un intercambio sobre diez dibujos se deshace como una operación.

**Alcance:** `base` · **Nivel:** `pro` · **Estado:** `not_started`.

**Referencia:** [H25-0717ca32d7](https://docs.toonboom.com/help/harmony-25/premium/paperless-animation/art-layer-manager.html) · `documentation_linked`.

## LYR-010 — Anotaciones de hoja

Añadir texto o dibujos de anotación independientes del render final.

**Aceptación inicial:** La anotación se ve en la hoja y no aparece en la secuencia exportada.

**Alcance:** `base` · **Nivel:** `pro` · **Estado:** `not_started`.

**Referencia:** [H25-099d14c6f9](https://docs.toonboom.com/help/harmony-25/premium/layers/about-annotation-column.html) · `documentation_linked`.

## LYR-011 — Edición múltiple de propiedades

Aplicar atributos comunes a una selección de capas compatible.

**Aceptación inicial:** La operación informa de capas incompatibles y conserva el resto.

**Alcance:** `base` · **Nivel:** `pro` · **Estado:** `not_started`.

**Referencia:** [H25-d55a19b732](https://docs.toonboom.com/help/harmony-25/premium/layers/set-property-many-layer.html) · `documentation_linked`.
