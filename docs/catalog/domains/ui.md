# UI — Espacio de trabajo, navegación y preferencias

[Volver al catálogo](../README.md)

> Vista generada desde `features.json` y `domains.json`; no editar a mano.

**Flujo:** Elegir espacio → dibujar o animar → personalizar paneles y atajos.

**Módulo:** `workspace`.

**Entidades:** Workspace, PanelLayout, ShortcutMap, ToolPreset.

**Relaciones:** PRJ.

**Riesgo principal:** Densidad profesional frente a legibilidad; estados de herramienta ambiguos.

## Contrato común

Las mutaciones deben respetar transacciones, undo/redo y persistencia. Las vistas de ayuda no se exportan. Errores, cancelación y datos no soportados deben conservar el último estado válido. Estas son condiciones de OPEN-TOON que se concretarán por operación al implementar.

## UI-001 — Paneles acoplables

Mover, dividir, agrupar y restablecer paneles sin perder el documento.

**Aceptación inicial:** Restaurar un layout deja todas las vistas accesibles.

**Alcance:** `base` · **Nivel:** `core` · **Estado:** `not_started`.

**Referencia:** [H25-c15782c56a](https://docs.toonboom.com/help/harmony-25/premium/user-interface/about-interface.html) · `documentation_linked`.

## UI-002 — Espacios guardados

Guardar y alternar configuraciones orientadas a dibujo, animación, rig y composición.

**Aceptación inicial:** Cambiar espacio conserva selección y fotograma.

**Alcance:** `base` · **Nivel:** `pro` · **Estado:** `not_started`.

**Referencia:** [H25-c15782c56a](https://docs.toonboom.com/help/harmony-25/premium/user-interface/about-interface.html) · `documentation_linked`.

## UI-003 — Barras personalizables

Mostrar herramientas relevantes y permitir ordenar acciones frecuentes.

**Aceptación inicial:** Un botón personalizado sigue funcionando al reiniciar.

**Alcance:** `base` · **Nivel:** `pro` · **Estado:** `not_started`.

**Referencia:** [H25-2a7e37b5db](https://docs.toonboom.com/help/harmony-25/premium/user-interface/about-toolbar.html) · `documentation_linked`.

## UI-004 — Atajos configurables

Asignar atajos por contexto con detección de conflictos y búsqueda de comandos.

**Aceptación inicial:** Un atajo en un campo de texto no dispara una acción destructiva.

**Alcance:** `base` · **Nivel:** `core` · **Estado:** `not_started`.

**Referencia:** [H25-c41ad42e04](https://docs.toonboom.com/help/harmony-25/premium/keyboard-shortcuts/about-keyboard-shortcuts.html) · `documentation_linked`.

## UI-005 — Preferencias persistentes

Separar preferencias personales, ajustes del documento y valores temporales.

**Aceptación inicial:** Cambiar tamaño de interfaz no modifica el archivo del proyecto.

**Alcance:** `base` · **Nivel:** `core` · **Estado:** `not_started`.

**Referencia:** [H25-b45f86a251](https://docs.toonboom.com/help/harmony-25/premium/preferences-guide/access-preference.html) · `documentation_linked`.

## UI-006 — Zoom, pan y rotación de vista

Navegar el lienzo sin alterar la geometría ni los keyframes.

**Aceptación inicial:** Rotar la vista 90 grados no cambia el render exportado.

**Alcance:** `base` · **Nivel:** `core` · **Estado:** `not_started`.

**Referencia:** [H25-b6961de319](https://docs.toonboom.com/help/harmony-25/premium/drawing/about-drawing-space.html) · `documentation_linked`.

## UI-007 — Espejo de vista

Invertir la visualización para inspección del dibujo sin reflejar los datos.

**Aceptación inicial:** Guardar con vista reflejada no invierte la imagen exportada.

**Alcance:** `base` · **Nivel:** `core` · **Estado:** `not_started`.

**Referencia:** [H25-1c10b08976](https://docs.toonboom.com/help/harmony-25/premium/drawing/about-mirror-view.html) · `documentation_linked`.

## UI-008 — Vistas de cámara y dibujo

Distinguir edición de dibujo aislado y resultado compuesto de la escena.

**Aceptación inicial:** El dibujo seleccionado se puede editar sin incluir efectos en sus datos.

**Alcance:** `base` · **Nivel:** `core` · **Estado:** `not_started`.

**Referencia:** [H25-b6961de319](https://docs.toonboom.com/help/harmony-25/premium/drawing/about-drawing-space.html) · `documentation_linked`.

## UI-009 — Inspector contextual

Mostrar atributos editables, su animación y sus unidades para la selección activa.

**Aceptación inicial:** La edición de varios objetos muestra valores mixtos correctamente.

**Alcance:** `base` · **Nivel:** `core` · **Estado:** `not_started`.

**Referencia:** [H25-3ef1042670](https://docs.toonboom.com/help/harmony-25/premium/staging/about-layer-property-view.html) · `documentation_linked`.

## UI-010 — Entrada con tableta

Usar presión y ajustes de sensibilidad; degradar de forma visible si el dispositivo no los soporta.

**Aceptación inicial:** Un mismo gesto varía el grosor según la curva de presión elegida.

**Alcance:** `base` · **Nivel:** `core` · **Estado:** `not_started`.

**Referencia:** [H25-62e4dfd23a](https://docs.toonboom.com/help/harmony-25/premium/drawing/about-pen-pressure-feel.html) · `documentation_linked`.
