# STU — Producción de estudio y colaboración

[Volver al catálogo](../README.md)

> Vista generada desde `features.json` y `domains.json`; no editar a mano.

**Flujo:** Asignar escena → obtener derechos → editar → versionar → entregar.

**Módulo:** `studio`.

**Entidades:** Production, Job, SceneCheckout, AssetLock, RenderQueue.

**Relaciones:** PRJ, LIB, OUT.

**Riesgo principal:** Conflictos multiusuario y red; no equivale a coedición CRDT.

## Contrato común

Las mutaciones deben respetar transacciones, undo/redo y persistencia. Las vistas de ayuda no se exportan. Errores, cancelación y datos no soportados deben conservar el último estado válido. Estas son condiciones de OPEN-TOON que se concretarán por operación al implementar.

## STU-001 — Producciones y trabajos

Organizar escenas en estructura de producción y asignaciones.

**Aceptación inicial:** Una escena tiene identidad independiente de su ruta en disco.

**Alcance:** `studio_extension` · **Nivel:** `advanced` · **Estado:** `not_started`.

**Referencia:** [H25-95ab978ba8](https://docs.toonboom.com/help/harmony-25/premium/project-creation/about-database-structure.html) · `documentation_linked`.

## STU-002 — Acceso a escenas compartidas

Abrir y guardar escenas en almacenamiento compartido mediante un servicio definido.

**Aceptación inicial:** Un usuario sin permisos recibe un rechazo sin cambios parciales.

**Alcance:** `studio_extension` · **Nivel:** `advanced` · **Estado:** `not_started`.

**Referencia:** [H25-2e940bb71d](https://docs.toonboom.com/help/harmony-25/premium/project-creation/about-database.html) · `documentation_linked`.

## STU-003 — Bloqueos de escena

Coordinar derechos de escritura y liberación de bloqueos.

**Aceptación inicial:** Dos clientes no sobrescriben la misma revisión silenciosamente.

**Alcance:** `studio_extension` · **Nivel:** `advanced` · **Estado:** `not_started`.

**Referencia:** [H25-dd30b0ab2c](https://docs.toonboom.com/help/harmony-25/premium/project-creation/about-global-lock.html) · `documentation_linked`.

## STU-004 — Bloqueos de dibujos

Permitir trabajo de pintura sobre dibujos autorizados mientras otros roles trabajan en escena.

**Aceptación inicial:** Un bloqueo de dibujo no impide editar un recurso distinto autorizado.

**Alcance:** `studio_extension` · **Nivel:** `advanced` · **Estado:** `not_started`.

**Referencia:** [H25-8cdb228d35](https://docs.toonboom.com/help/harmony-25/premium/project-creation/about-drawing-lock.html) · `documentation_linked`.

## STU-005 — Versiones y merge de entregas

Gestionar revisiones y detectar divergencias al reintegrar una escena.

**Aceptación inicial:** Una entrega antigua no sustituye una nueva sin resolver el conflicto.

**Alcance:** `studio_extension` · **Nivel:** `advanced` · **Estado:** `not_started`.

**Referencia:** [H25-523ce57039](https://docs.toonboom.com/help/harmony-25/premium/project-creation/merge-scene-version-harmony-srever.html) · `documentation_linked`.

## STU-006 — Checkout remoto

Empaquetar escenas para trabajo desconectado y reintegrar cambios.

**Aceptación inicial:** Un checkout conserva una revisión base que permite detectar conflictos.

**Alcance:** `studio_extension` · **Nivel:** `advanced` · **Estado:** `not_started`.

**Referencia:** [H25-acfe671ce8](https://docs.toonboom.com/help/harmony-25/premium/about/about.html) · `documentation_linked`.

## STU-007 — Colas de render y vectorización

Distribuir trabajos con estado, reintento y seguimiento.

**Aceptación inicial:** Un worker que desaparece deja un trabajo recuperable.

**Alcance:** `studio_extension` · **Nivel:** `advanced` · **Estado:** `not_started`.

**Referencia:** [H25-2e940bb71d](https://docs.toonboom.com/help/harmony-25/premium/project-creation/about-database.html) · `documentation_linked`.

## STU-008 — Roles Paint y Scan

Ofrecer espacios limitados por tarea para pintura y escaneo de producción.

**Aceptación inicial:** El espacio de pintura no expone comandos que alteren el timing protegido.

**Alcance:** `studio_extension` · **Nivel:** `advanced` · **Estado:** `not_started`.

**Referencia:** [H25-acfe671ce8](https://docs.toonboom.com/help/harmony-25/premium/about/about.html) · `documentation_linked`.
