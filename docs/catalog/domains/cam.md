# CAM — Cámara, staging y espacio 2.5D

[Volver al catálogo](../README.md)

> Vista generada desde `features.json` y `domains.json`; no editar a mano.

**Flujo:** Colocar planos → encuadrar → animar cámara → comprobar composición.

**Módulo:** `camera`.

**Entidades:** Camera, Projection, StagePlane, SafeArea.

**Relaciones:** ANI.

**Riesgo principal:** Proyección y profundidad deben coincidir entre vistas.

## Contrato común

Las mutaciones deben respetar transacciones, undo/redo y persistencia. Las vistas de ayuda no se exportan. Errores, cancelación y datos no soportados deben conservar el último estado válido. Estas son condiciones de OPEN-TOON que se concretarán por operación al implementar.

## CAM-001 — Cámaras y cámara activa

Crear cámaras y seleccionar explícitamente la que produce la salida.

**Aceptación inicial:** Cambiar cámara activa cambia preview y exportación de forma concordante.

**Alcance:** `base` · **Nivel:** `core` · **Estado:** `not_started`.

**Referencia:** [H25-c7e69774d0](https://docs.toonboom.com/help/harmony-25/premium/camera/about-camera.html) · `documentation_linked`.

## CAM-002 — Transformación de cámara

Animar posición, rotación y encuadre con pegs y curvas.

**Aceptación inicial:** Un movimiento de cámara no modifica coordenadas locales de dibujos.

**Alcance:** `base` · **Nivel:** `pro` · **Estado:** `not_started`.

**Referencia:** [H25-c7e69774d0](https://docs.toonboom.com/help/harmony-25/premium/camera/about-camera.html) · `documentation_linked`.

## CAM-003 — Multiplano

Distribuir capas en profundidad para generar paralaje.

**Aceptación inicial:** Un travelling produce desplazamientos diferentes según profundidad.

**Alcance:** `base` · **Nivel:** `pro` · **Estado:** `not_started`.

**Referencia:** [H25-eceb3298a3](https://docs.toonboom.com/help/harmony-25/premium/staging/about-multiplane.html) · `documentation_linked`.

## CAM-004 — Vistas superior, lateral y perspectiva

Inspeccionar posiciones espaciales y relaciones de cámara.

**Aceptación inicial:** Una selección corresponde al mismo objeto en las distintas vistas.

**Alcance:** `base` · **Nivel:** `pro` · **Estado:** `not_started`.

**Referencia:** [H25-ce4ab163a0](https://docs.toonboom.com/help/harmony-25/premium/3d-integration/about-3d-object-different-view.html) · `documentation_linked`.

## CAM-005 — Proyección ortográfica

Ofrecer salida sin perspectiva para flujos 2D y videojuegos.

**Aceptación inicial:** Objetos iguales a distinta profundidad mantienen tamaño aparente.

**Alcance:** `base` · **Nivel:** `pro` · **Estado:** `not_started`.

**Referencia:** [H25-69025b6c3f](https://docs.toonboom.com/help/harmony-25/premium/gaming/about-orthographic-camera.html) · `documentation_linked`.

## CAM-006 — Guías de encuadre

Usar guías de alineación y zonas de referencia como ayudas no exportables.

**Aceptación inicial:** Las guías no aparecen en la imagen renderizada.

**Alcance:** `base` · **Nivel:** `core` · **Estado:** `not_started`.

**Referencia:** [H25-8174da4d99](https://docs.toonboom.com/help/harmony-25/premium/staging/about-alignment-guides.html) · `documentation_linked`.

## CAM-007 — Transformación y opacidad de staging

Posicionar fondos y referencias con bloqueo y transparencia temporal.

**Aceptación inicial:** Cambiar opacidad de trabajo se distingue de opacidad renderizable.

**Alcance:** `base` · **Nivel:** `core` · **Estado:** `not_started`.

**Referencia:** [H25-fdba21b1cf](https://docs.toonboom.com/help/harmony-25/premium/staging/about-opacity-transparency.html) · `documentation_linked`.
