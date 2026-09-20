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

**Evidencia:** `proposal`.

## PRJ-002 — Resolución y aspect ratio

Configurar anchura, altura, proporción de píxel y presets personalizados.

**Aceptación inicial:** Un preset no cuadrado reproduce el encuadre esperado.

**Alcance:** `base` · **Nivel:** `core` · **Estado:** `not_started`.

**Evidencia:** `proposal`.

## PRJ-003 — FPS y duración

Definir tasa de fotogramas y longitud; decidir si cambiar FPS conserva frames o tiempo.

**Aceptación inicial:** Cambiar de 24 a 25 fps informa del efecto temporal y permite deshacer.

**Alcance:** `base` · **Nivel:** `core` · **Estado:** `not_started`.

**Evidencia:** `proposal`.

## PRJ-004 — Guardar y guardar copia

Persistir recursos y escena con una operación coherente; guardar copia conserva el original.

**Aceptación inicial:** La copia se abre desde otra ruta sin depender del directorio original.

**Alcance:** `base` · **Nivel:** `core` · **Estado:** `not_started`.

**Evidencia:** `proposal`.

## PRJ-005 — Autoguardado

Programar recuperación sin bloquear dibujo ni sustituir silenciosamente un guardado manual.

**Aceptación inicial:** Un cierre forzado permite recuperar el último punto de recuperación válido.

**Alcance:** `base` · **Nivel:** `core` · **Estado:** `not_started`.

**Evidencia:** `proposal`.

## PRJ-006 — Versiones de escena

Conservar revisiones identificables y abrir una versión elegida.

**Aceptación inicial:** Una revisión anterior mantiene sus dibujos aunque la nueva cambie.

**Alcance:** `base` · **Nivel:** `pro` · **Estado:** `not_started`.

**Evidencia:** `proposal`.

## PRJ-007 — Formato compacto

Ofrecer empaquetado y desempaquetado de escena con recursos incluidos.

**Aceptación inicial:** Ambas representaciones producen la misma imagen al reabrirse.

**Alcance:** `base` · **Nivel:** `pro` · **Estado:** `not_started`.

**Evidencia:** `proposal`.

## PRJ-008 — Integridad de proyecto

Detectar archivos ausentes, referencias inválidas y componentes corruptos con diagnóstico accionable.

**Aceptación inicial:** Una textura perdida aparece en un informe y no provoca cierre del editor.

**Alcance:** `base` · **Nivel:** `core` · **Estado:** `not_started`.

**Evidencia:** `proposal`.

## PRJ-009 — Optimización de backups

Listar tamaño y uso de copias antes de limpiar recursos redundantes.

**Aceptación inicial:** No eliminar un recurso que siga referenciado por otra revisión.

**Alcance:** `base` · **Nivel:** `pro` · **Estado:** `not_started`.

**Evidencia:** `proposal`.

## PRJ-010 — Recientes y bienvenida

Mostrar escenas recientes y acciones de crear, abrir y localizar un archivo movido.

**Aceptación inicial:** Un reciente inexistente puede retirarse sin borrar otros archivos.

**Alcance:** `base` · **Nivel:** `core` · **Estado:** `not_started`.

**Evidencia:** `proposal`.
