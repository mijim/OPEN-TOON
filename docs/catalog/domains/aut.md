# AUT — Scripting y automatización

[Volver al catálogo](../README.md)

> Vista generada desde `features.json` y `domains.json`; no editar a mano.

**Flujo:** Automatizar edición → ejecutar comandos → procesar lotes → integrar pipeline.

**Módulo:** `scripting`.

**Entidades:** CommandAPI, Script, ScriptContext, BatchJob.

**Relaciones:** PRJ, NOD, OUT.

**Riesgo principal:** Código externo ejecutable y cambios no reversibles.

## Contrato común

Las mutaciones deben respetar transacciones, undo/redo y persistencia. Las vistas de ayuda no se exportan. Errores, cancelación y datos no soportados deben conservar el último estado válido. Estas son condiciones de OPEN-TOON que se concretarán por operación al implementar.

## AUT-001 — Editor y ejecución de scripts

Crear y ejecutar scripts con salida, errores y contexto de escena explícitos.

**Aceptación inicial:** Un error indica script y posición y preserva el último estado válido.

**Alcance:** `base` · **Nivel:** `pro` · **Estado:** `not_started`.

**Referencia:** [H25-cb193b7df9](https://docs.toonboom.com/help/harmony-25/premium/scripting/about-scripting.html) · `documentation_linked`.

## AUT-002 — Acciones de script en UI

Asignar scripts a botones y acciones identificables.

**Aceptación inicial:** Una acción inexistente se muestra deshabilitada con causa.

**Alcance:** `base` · **Nivel:** `pro` · **Estado:** `not_started`.

**Referencia:** [H25-4676463c4a](https://docs.toonboom.com/help/harmony-25/premium/scripting/link-script-button.html) · `documentation_linked`.

## AUT-003 — API de escena y nodos

Exponer consultas y comandos sobre capas, atributos, dibujos y conexiones.

**Aceptación inicial:** Una modificación por API actualiza las mismas vistas que la UI.

**Alcance:** `base` · **Nivel:** `pro` · **Estado:** `not_started`.

**Referencia:** [H25-362b8b5ece](https://docs.toonboom.com/help/harmony-25/premium/scripting/show-node-attributes.html) · `documentation_linked`.

## AUT-004 — Integración Python

Facilitar automatización externa con una API versionada y contratos de error.

**Aceptación inicial:** Un proceso externo puede abrir una copia, consultar y renderizar sin UI.

**Alcance:** `base` · **Nivel:** `advanced` · **Estado:** `not_started`.

**Referencia:** [H25-e7577ec800](https://docs.toonboom.com/help/harmony-25/premium/release-notes/harmony/harmony-25-release-notes.html) · `documentation_linked`.

## AUT-005 — Sandbox y contexto de ejecución

Separar pruebas de scripts y políticas de ejecución del documento.

**Aceptación inicial:** Abrir una escena ajena no ejecuta scripts por sí solo.

**Alcance:** `base` · **Nivel:** `pro` · **Estado:** `not_started`.

**Referencia:** [H25-699184a3ab](https://docs.toonboom.com/help/harmony-25/premium/scripting/use-sandbox.html) · `documentation_linked`.

## AUT-006 — Herramientas de conversión CLI

Exponer validación, conversión y render como comandos con resultados estructurados.

**Aceptación inicial:** Un archivo inválido devuelve código no cero y diagnóstico legible.

**Alcance:** `base` · **Nivel:** `pro` · **Estado:** `not_started`.

**Referencia:** [H25-aa2ed4e9df](https://docs.toonboom.com/help/harmony-25/premium/utilities/introduction-utilities.html) · `documentation_linked`.
