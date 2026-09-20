# THR — Integración 3D

[Volver al catálogo](../README.md)

> Vista generada desde `features.json` y `domains.json`; no editar a mano.

**Flujo:** Importar modelo → ajustar unidades → posar → componer con 2D.

**Módulo:** `integration-3d`.

**Entidades:** ModelAsset, ModelSubnode, Skeleton, AnimationClip, ExternalRenderJob.

**Relaciones:** CAM, NOD.

**Riesgo principal:** Compatibilidad de formatos, ejes y coste del render externo.

## Contrato común

Las mutaciones deben respetar transacciones, undo/redo y persistencia. Las vistas de ayuda no se exportan. Errores, cancelación y datos no soportados deben conservar el último estado válido. Estas son condiciones de OPEN-TOON que se concretarán por operación al implementar.

## THR-001 — Importar modelos 3D

Leer formatos admitidos con materiales y jerarquías dentro de un perfil documentado.

**Aceptación inicial:** Un modelo de referencia conserva las proporciones y orientación.

**Alcance:** `base` · **Nivel:** `advanced` · **Estado:** `not_started`.

**Evidencia:** `proposal`.

## THR-002 — Unidades y escala

Interpretar unidades del archivo y convertirlas al espacio de la escena.

**Aceptación inicial:** Un objeto de un metro mantiene relación diez a uno con uno de diez centímetros.

**Alcance:** `base` · **Nivel:** `advanced` · **Estado:** `not_started`.

**Evidencia:** `proposal`.

## THR-003 — Clips de animación 3D

Seleccionar y retimar animaciones incluidas en el asset.

**Aceptación inicial:** Un clip repetido no salta en sus límites cuando es cíclico.

**Alcance:** `base` · **Nivel:** `advanced` · **Estado:** `not_started`.

**Evidencia:** `proposal`.

## THR-004 — Subnodos

Seleccionar y animar componentes internos del modelo.

**Aceptación inicial:** Mover una pieza no desplaza una pieza hermana sin vínculo.

**Alcance:** `base` · **Nivel:** `advanced` · **Estado:** `not_started`.

**Evidencia:** `proposal`.

## THR-005 — Armatures y skinning

Animar esqueletos y deformación del modelo bajo límites soportados.

**Aceptación inicial:** El rig de prueba conserva la pose de bind al importar.

**Alcance:** `base` · **Nivel:** `advanced` · **Estado:** `not_started`.

**Evidencia:** `proposal`.

## THR-006 — Overrides de subnodos

Hornear transformaciones y clips importados en claves editables.

**Aceptación inicial:** El bake reproduce las posiciones muestreadas del clip original.

**Alcance:** `base` · **Nivel:** `advanced` · **Estado:** `not_started`.

**Evidencia:** `proposal`.

## THR-007 — Composición 2D y 3D

Combinar planos dibujados y modelos con profundidad, transparencia y cámara coherentes.

**Aceptación inicial:** Un objeto atraviesa planos con el orden esperado.

**Alcance:** `base` · **Nivel:** `advanced` · **Estado:** `not_started`.

**Evidencia:** `proposal`.

## THR-008 — Render externo

Orquestar renderizadores externos con cancelación, errores y recursos explícitos.

**Aceptación inicial:** Un fallo del proceso de render no cierra el editor.

**Alcance:** `base` · **Nivel:** `advanced` · **Estado:** `not_started`.

**Evidencia:** `proposal`.

## THR-009 — Normalización de profundidad

Tratar bordes de Z y aplanado de resultados 3D.

**Aceptación inicial:** La composición de borde no produce halos por profundidad inválida.

**Alcance:** `base` · **Nivel:** `advanced` · **Estado:** `not_started`.

**Evidencia:** `proposal`.
