# NOD — Grafo de composición

[Volver al catálogo](../README.md)

> Vista generada desde `features.json` y `domains.json`; no editar a mano.

**Flujo:** Conectar entradas → agrupar → parametrizar → previsualizar → renderizar.

**Módulo:** `compositor-graph`.

**Entidades:** Node, Port, Edge, NodeGroup, AttributeBinding.

**Relaciones:** LYR, ANI.

**Riesgo principal:** Ciclos, invalidación incorrecta y diferencia entre orden de capas y topología.

## Contrato común

Las mutaciones deben respetar transacciones, undo/redo y persistencia. Las vistas de ayuda no se exportan. Errores, cancelación y datos no soportados deben conservar el último estado válido. Estas son condiciones de OPEN-TOON que se concretarán por operación al implementar.

## NOD-001 — Vista de nodos

Crear, mover, conectar, buscar y eliminar nodos con navegación del grafo.

**Aceptación inicial:** Eliminar un nodo reconecta solo según la opción elegida.

**Alcance:** `base` · **Nivel:** `pro` · **Estado:** `not_started`.

**Evidencia:** `proposal`.

## NOD-002 — Puertos tipados

Distinguir imagen, transformación, matte y otros flujos de datos.

**Aceptación inicial:** Una conexión incompatible se rechaza antes de evaluar.

**Alcance:** `base` · **Nivel:** `pro` · **Estado:** `not_started`.

**Evidencia:** `proposal`.

## NOD-003 — Biblioteca de nodos

Organizar operadores por categoría y búsqueda con descripción.

**Aceptación inicial:** La búsqueda encuentra un operador por su nombre y categoría.

**Alcance:** `base` · **Nivel:** `pro` · **Estado:** `not_started`.

**Evidencia:** `proposal`.

## NOD-004 — Grupos y puertos publicados

Encapsular subgrafos con entradas y salidas reutilizables.

**Aceptación inicial:** Agrupar mantiene la imagen resultante del grafo.

**Alcance:** `base` · **Nivel:** `pro` · **Estado:** `not_started`.

**Evidencia:** `proposal`.

## NOD-005 — Composición por capas

Combinar entradas manteniendo orden, alfa y profundidad según el modo.

**Aceptación inicial:** Una capa semitransparente produce el alfa esperado sobre fondo transparente.

**Alcance:** `base` · **Nivel:** `core` · **Estado:** `not_started`.

**Evidencia:** `proposal`.

## NOD-006 — Máscaras y cutters

Recortar por matte con inversión y tratamiento de alfa definidos.

**Aceptación inicial:** Una máscara parcial no se interpreta como binaria salvo modo explícito.

**Alcance:** `base` · **Nivel:** `core` · **Estado:** `not_started`.

**Evidencia:** `proposal`.

## NOD-007 — Switches

Seleccionar imágenes o transformaciones según atributos animados.

**Aceptación inicial:** Cambiar selector no evalúa ramas innecesarias salvo dependencia compartida.

**Alcance:** `base` · **Nivel:** `pro` · **Estado:** `not_started`.

**Evidencia:** `proposal`.

## NOD-008 — Display y Write

Separar salida de preview y salidas finales exportables.

**Aceptación inicial:** Un Display alternativo no cambia el Write configurado.

**Alcance:** `base` · **Nivel:** `pro` · **Estado:** `not_started`.

**Evidencia:** `proposal`.

## NOD-009 — Bypass y cache

Anular temporalmente operadores y reutilizar resultados válidos.

**Aceptación inicial:** Editar una dependencia invalida todas las salidas afectadas.

**Alcance:** `base` · **Nivel:** `pro` · **Estado:** `not_started`.

**Evidencia:** `proposal`.

## NOD-010 — Notas y organización

Añadir notas y organización espacial sin influencia en la imagen.

**Aceptación inicial:** Mover notas no invalida el render.

**Alcance:** `base` · **Nivel:** `pro` · **Estado:** `not_started`.

**Evidencia:** `proposal`.
