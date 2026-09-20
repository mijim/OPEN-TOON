# PRJ — Proyectos, escenas y persistencia

[Volver al catálogo](../README.md)

> Vista generada desde `features.json` y `domains.json`; no editar a mano.

**Flujo:** Crear escena → configurar → guardar → recuperar → empaquetar.

**Módulo:** `project`.

**Entidades:** Project, Scene, SceneSettings, AssetRef, Revision.

**Relaciones:** Ninguna.

**Riesgo principal:** Pérdida de datos y referencias rotas.

## Contrato común

Las mutaciones deben respetar transacciones, undo/redo y persistencia. Las vistas de ayuda no se exportan. Errores, cancelación y datos no soportados deben conservar el último estado válido. Estas son condiciones de OPEN-TOON que se concretarán por operación al implementar.

## PRJ-001 — Crear y abrir escenas

Crear escenas locales con nombre, ruta y configuración explícita.

**Aceptación inicial:** Reabrir una escena nueva conserva dimensiones y duración.

**Alcance:** `base` · **Nivel:** `core` · **Estado:** `not_started`.

**Referencia:** [H25-3bafbe2bec](https://docs.toonboom.com/help/harmony-25/premium/project-creation/create-scene.html) · `documentation_linked`.

## PRJ-002 — Resolución y aspect ratio

Configurar anchura, altura, proporción de píxel y presets personalizados.

**Aceptación inicial:** Un preset no cuadrado reproduce el encuadre esperado.

**Alcance:** `base` · **Nivel:** `core` · **Estado:** `not_started`.

**Referencia:** [H25-da6b6cf1b2](https://docs.toonboom.com/help/harmony-25/premium/project-creation/create-custom-resolution.html) · `documentation_linked`.

## PRJ-003 — FPS y duración

Definir tasa de fotogramas y longitud; decidir si cambiar FPS conserva frames o tiempo.

**Aceptación inicial:** Cambiar de 24 a 25 fps informa del efecto temporal y permite deshacer.

**Alcance:** `base` · **Nivel:** `core` · **Estado:** `not_started`.

**Referencia:** [H25-8dacf5486c](https://docs.toonboom.com/help/harmony-25/premium/project-creation/about-scene-setting.html) · `documentation_linked`.

## PRJ-004 — Guardar y guardar copia

Persistir recursos y escena con una operación coherente; guardar copia conserva el original.

**Aceptación inicial:** La copia se abre desde otra ruta sin depender del directorio original.

**Alcance:** `base` · **Nivel:** `core` · **Estado:** `not_started`.

**Referencia:** [H25-fc0d10d142](https://docs.toonboom.com/help/harmony-25/premium/project-creation/save-scene-new-copy.html) · `documentation_linked`.

## PRJ-005 — Autoguardado

Programar recuperación sin bloquear dibujo ni sustituir silenciosamente un guardado manual.

**Aceptación inicial:** Un cierre forzado permite recuperar el último punto de recuperación válido.

**Alcance:** `base` · **Nivel:** `core` · **Estado:** `not_started`.

**Referencia:** [H25-6927a4c994](https://docs.toonboom.com/help/harmony-25/premium/project-creation/auto-save-scene.html) · `documentation_linked`.

## PRJ-006 — Versiones de escena

Conservar revisiones identificables y abrir una versión elegida.

**Aceptación inicial:** Una revisión anterior mantiene sus dibujos aunque la nueva cambie.

**Alcance:** `base` · **Nivel:** `pro` · **Estado:** `not_started`.

**Referencia:** [H25-3f5fbd4b66](https://docs.toonboom.com/help/harmony-25/premium/project-creation/save-scene-new-version.html) · `documentation_linked`.

## PRJ-007 — Formato compacto

Ofrecer empaquetado y desempaquetado de escena con recursos incluidos.

**Aceptación inicial:** Ambas representaciones producen la misma imagen al reabrirse.

**Alcance:** `base` · **Nivel:** `pro` · **Estado:** `not_started`.

**Referencia:** [H25-1ce1dd2d1e](https://docs.toonboom.com/help/harmony-25/premium/project-creation/compact-file-structure.html) · `documentation_linked`.

## PRJ-008 — Integridad de proyecto

Detectar archivos ausentes, referencias inválidas y componentes corruptos con diagnóstico accionable.

**Aceptación inicial:** Una textura perdida aparece en un informe y no provoca cierre del editor.

**Alcance:** `base` · **Nivel:** `core` · **Estado:** `not_started`.

**Referencia:** [H25-4b1d0004a4](https://docs.toonboom.com/help/harmony-25/premium/project-creation/verify-project-integrity.html) · `documentation_linked`.

## PRJ-009 — Optimización de backups

Listar tamaño y uso de copias antes de limpiar recursos redundantes.

**Aceptación inicial:** No eliminar un recurso que siga referenciado por otra revisión.

**Alcance:** `base` · **Nivel:** `pro` · **Estado:** `not_started`.

**Referencia:** [H25-3c91b9aef0](https://docs.toonboom.com/help/harmony-25/premium/release-notes/harmony/harmony-25-1-release-notes.html) · `documentation_linked`.

## PRJ-010 — Recientes y bienvenida

Mostrar escenas recientes y acciones de crear, abrir y localizar un archivo movido.

**Aceptación inicial:** Un reciente inexistente puede retirarse sin borrar otros archivos.

**Alcance:** `base` · **Nivel:** `core` · **Estado:** `not_started`.

**Referencia:** [H25-ba4fd0aa8d](https://docs.toonboom.com/help/harmony-25/premium/project-creation/about-welcome-screen.html) · `documentation_linked`.
