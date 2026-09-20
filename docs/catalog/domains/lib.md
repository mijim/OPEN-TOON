# LIB — Bibliotecas, símbolos y reutilización

[Volver al catálogo](../README.md)

> Vista generada desde `features.json` y `domains.json`; no editar a mano.

**Flujo:** Crear asset → catalogar → reutilizar → actualizar.

**Módulo:** `asset-library`.

**Entidades:** AssetLibrary, Template, Symbol, AssetVersion.

**Relaciones:** PRJ, RIG.

**Riesgo principal:** Assets que parecen independientes pero comparten datos.

## Contrato común

Las mutaciones deben respetar transacciones, undo/redo y persistencia. Las vistas de ayuda no se exportan. Errores, cancelación y datos no soportados deben conservar el último estado válido. Estas son condiciones de OPEN-TOON que se concretarán por operación al implementar.

## LIB-001 — Bibliotecas locales

Organizar assets por carpetas con búsqueda, previews y refresco.

**Aceptación inicial:** Mover el catálogo no pierde las referencias empaquetadas.

**Alcance:** `base` · **Nivel:** `core` · **Estado:** `not_started`.

**Evidencia:** `proposal`.

## LIB-002 — Plantillas de escenas y rigs

Guardar selecciones con dibujos, paletas, nodos y animación requerida.

**Aceptación inicial:** Una plantilla se importa correctamente en una escena vacía.

**Alcance:** `base` · **Nivel:** `pro` · **Estado:** `not_started`.

**Evidencia:** `proposal`.

## LIB-003 — Símbolos reutilizables

Encapsular elementos con su propio contenido y transformación.

**Aceptación inicial:** Editar un símbolo actualiza sus instancias según el modo de enlace.

**Alcance:** `base` · **Nivel:** `pro` · **Estado:** `not_started`.

**Evidencia:** `proposal`.

## LIB-004 — Importar plantillas

Aplicar reglas de copia, enlaces y conflictos al reutilizar assets.

**Aceptación inicial:** Dos imports pueden ser independientes si se elige duplicación.

**Alcance:** `base` · **Nivel:** `pro` · **Estado:** `not_started`.

**Evidencia:** `proposal`.

## LIB-005 — Paste special

Seleccionar si se importan animación, dibujos, paletas o estructura.

**Aceptación inicial:** Pegar únicamente claves no sobrescribe el rig receptor.

**Alcance:** `base` · **Nivel:** `pro` · **Estado:** `not_started`.

**Evidencia:** `proposal`.

## LIB-006 — Miniaturas de biblioteca

Generar previews actualizables sin bloquear edición.

**Aceptación inicial:** Un asset modificado invalida su miniatura.

**Alcance:** `base` · **Nivel:** `pro` · **Estado:** `not_started`.

**Evidencia:** `proposal`.

## LIB-007 — Edición de plantillas

Editar un recurso de biblioteca distinguiendo original e instancia.

**Aceptación inicial:** Guardar cambios indica qué recurso se modifica.

**Alcance:** `base` · **Nivel:** `pro` · **Estado:** `not_started`.

**Evidencia:** `proposal`.
