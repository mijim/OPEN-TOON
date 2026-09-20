# AIX — Asistencia de IA opcional

[Volver al catálogo](../README.md)

> Vista generada desde `features.json` y `domains.json`; no editar a mano.

**Flujo:** Seleccionar → configurar proveedor → generar → comparar → aceptar.

**Módulo:** `ai-adapters`.

**Entidades:** Provider, InferenceJob, Mask, GeneratedVariant.

**Relaciones:** RAS, PRJ.

**Riesgo principal:** Dependencia de servicios, licencias de modelos y pérdida de control creativo.

## Contrato común

Las mutaciones deben respetar transacciones, undo/redo y persistencia. Las vistas de ayuda no se exportan. Errores, cancelación y datos no soportados deben conservar el último estado válido. Estas son condiciones de OPEN-TOON que se concretarán por operación al implementar.

## AIX-001 — Habilitación opcional

Aislar funciones de IA para que todo el editor principal funcione sin ellas.

**Aceptación inicial:** Deshabilitar IA no bloquea abrir, dibujar o exportar proyectos.

**Alcance:** `optional_extension` · **Nivel:** `optional` · **Estado:** `not_started`.

**Referencia:** [H25-e3cb778b39](https://docs.toonboom.com/help/harmony-25/premium/ai/ai-features.html) · `documentation_linked`.

## AIX-002 — Máscaras de IA

Crear, modificar y borrar máscaras con expansión controlada.

**Aceptación inicial:** La operación afecta solo a la máscara y al padding elegido.

**Alcance:** `optional_extension` · **Nivel:** `optional` · **Estado:** `not_started`.

**Referencia:** [H25-7357c304d0](https://docs.toonboom.com/help/harmony-25/premium/ai/ai-masking.html) · `documentation_linked`.

## AIX-003 — Borrado asistido

Generar una propuesta para eliminar contenido enmascarado sin destruir el original.

**Aceptación inicial:** Rechazar una variante restaura exactamente la imagen de entrada.

**Alcance:** `optional_extension` · **Nivel:** `optional` · **Estado:** `not_started`.

**Referencia:** [H25-6c35c5ea37](https://docs.toonboom.com/help/harmony-25/premium/ai/ai-erase.html) · `documentation_linked`.

## AIX-004 — Expansión de imagen

Generar contenido más allá de los límites manteniendo el original identificable.

**Aceptación inicial:** La imagen original mantiene su registro tras ampliar el lienzo.

**Alcance:** `optional_extension` · **Nivel:** `optional` · **Estado:** `not_started`.

**Referencia:** [H25-b2ac012495](https://docs.toonboom.com/help/harmony-25/premium/ai/expand-image.html) · `documentation_linked`.

## AIX-005 — Aumento de resolución

Proponer imágenes o texturas de mayor resolución como variantes.

**Aceptación inicial:** El resultado informa del factor y conserva una copia de la entrada.

**Alcance:** `optional_extension` · **Nivel:** `optional` · **Estado:** `not_started`.

**Referencia:** [H25-44c8e3fbea](https://docs.toonboom.com/help/harmony-25/premium/ai/increase-image-resolution.html) · `documentation_linked`.

## AIX-006 — Relleno generativo

Generar contenido en una región guiado por texto con aceptación manual.

**Aceptación inicial:** La generación no sustituye el recurso hasta aceptarse.

**Alcance:** `optional_extension` · **Nivel:** `optional` · **Estado:** `not_started`.

**Referencia:** [H25-7a85f4304a](https://docs.toonboom.com/help/harmony-25/premium/ai/ai-generative-fill.html) · `documentation_linked`.
