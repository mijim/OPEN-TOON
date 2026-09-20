# CTL — Controladores avanzados de personajes

[Volver al catálogo](../README.md)

> Vista generada desde `features.json` y `domains.json`; no editar a mano.

**Flujo:** Capturar poses → definir controles → interpolar → empaquetar.

**Módulo:** `controllers`.

**Entidades:** Controller, PoseSample, ControlBinding, ControllerWidget.

**Relaciones:** DEF, ANI.

**Riesgo principal:** Una interpolación de poses no inventa dibujos ni resuelve topología incompatible.

## Contrato común

Las mutaciones deben respetar transacciones, undo/redo y persistencia. Las vistas de ayuda no se exportan. Errores, cancelación y datos no soportados deben conservar el último estado válido. Estas son condiciones de OPEN-TOON que se concretarán por operación al implementar.

## CTL-001 — Controles en cámara

Mostrar widgets vinculados a atributos del rig con selección y límites.

**Aceptación inicial:** Arrastrar un widget cambia solo sus atributos enlazados.

**Alcance:** `base` · **Nivel:** `advanced` · **Estado:** `not_started`.

**Referencia:** [H25-25b190db56](https://docs.toonboom.com/help/harmony-25/premium/master-controller/about-master-controller.html) · `documentation_linked`.

## CTL-002 — Slider de poses

Generar un control unidimensional a partir de poses compatibles.

**Aceptación inicial:** En cada muestra el controlador reproduce exactamente la pose capturada.

**Alcance:** `base` · **Nivel:** `advanced` · **Estado:** `not_started`.

**Referencia:** [H25-f3f4f3f583](https://docs.toonboom.com/help/harmony-25/premium/master-controller/about-slider-wizard.html) · `documentation_linked`.

## CTL-003 — Rejilla de poses

Crear un control bidimensional sobre muestras de pose organizadas.

**Aceptación inicial:** Los puntos de la rejilla reproducen sus muestras y las celdas interpolan.

**Alcance:** `base` · **Nivel:** `advanced` · **Estado:** `not_started`.

**Referencia:** [H25-c40cb9f32b](https://docs.toonboom.com/help/harmony-25/premium/master-controller/about-grid-wizard.html) · `documentation_linked`.

## CTL-004 — Asistente de funciones

Enlazar controles a atributos, activación y visibilidad sin escribir cada script.

**Aceptación inicial:** Un interruptor puede mostrar un conjunto de controles sin alterar el render.

**Alcance:** `base` · **Nivel:** `advanced` · **Estado:** `not_started`.

**Referencia:** [H25-4d847c3493](https://docs.toonboom.com/help/harmony-25/premium/master-controller/about-function-wizard.html) · `documentation_linked`.

## CTL-005 — Panel de personaje

Agrupar controles identificables para manipulación de un personaje.

**Aceptación inicial:** Duplicar el personaje crea bindings independientes.

**Alcance:** `base` · **Nivel:** `advanced` · **Estado:** `not_started`.

**Referencia:** [H25-a9027b8149](https://docs.toonboom.com/help/harmony-25/premium/master-controller/function-wizard-create-character-dashboard.html) · `documentation_linked`.

## CTL-006 — Controlador por script

Exponer eventos y atributos para controles personalizados con validación.

**Aceptación inicial:** Un error de script se informa sin corromper la escena.

**Alcance:** `base` · **Nivel:** `advanced` · **Estado:** `not_started`.

**Referencia:** [H25-8f84bf9cad](https://docs.toonboom.com/help/harmony-25/premium/master-controller/script-master-controller.html) · `documentation_linked`.

## CTL-007 — Deformador sobre deformador

Componer controles que actúan sobre otra estructura de deformación.

**Aceptación inicial:** La evaluación respeta el orden declarado y rechaza ciclos.

**Alcance:** `base` · **Nivel:** `advanced` · **Estado:** `not_started`.

**Referencia:** [H25-b74f4903b0](https://docs.toonboom.com/help/harmony-25/premium/master-controller/about-deformer-on-deformer.html) · `documentation_linked`.

## CTL-008 — Controladores transportables

Incluir dependencias y bindings al guardar una plantilla de rig.

**Aceptación inicial:** Importar la plantilla en otra escena conserva controles operativos.

**Alcance:** `base` · **Nivel:** `advanced` · **Estado:** `not_started`.

**Referencia:** [H25-e69bbb249a](https://docs.toonboom.com/help/harmony-25/premium/master-controller/create-template-master-controller.html) · `documentation_linked`.
