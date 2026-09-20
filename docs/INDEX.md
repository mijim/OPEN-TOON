# Mapa del análisis

## Decisiones principales

| Tema | Resultado | Estado |
|---|---|---|
| Producto propio | Editor profesional 2D, local y de escritorio | Recomendación |
| Estética | Blanco, negro y grises; lenguaje visual Geist | Solicitado |
| Tecnología | C++20, Qt 6 y Qt Quick/QML | Recomendación condicionada a pruebas |
| Motor | Modelo propio; backend de render aislado, candidatos Qt RHI/Skia | Decisión pendiente de medición |
| Persistencia | Formato abierto versionado, recursos por hash, guardado transaccional | Diseño propuesto |
| Licencia inicial | GPL-3.0-or-later para el repositorio | Adoptada para esta publicación |
| Implementación | Sin aplicación; catálogo íntegramente `not_started` | Hecho |
| Próxima etapa | Plan detallado por dependencias y entregables verificables | Pendiente de revisión del análisis |

## Rutas de lectura

**Producto:** [alcance](research/01-scope-and-method.md) → [catálogo](catalog/README.md) → [flujos de trabajo](research/02-workflows.md) → [riesgos](research/03-risks-and-decisions.md).

**Ingeniería:** [tecnologías](architecture/01-technology-selection.md) → [arquitectura](architecture/02-system-design.md) → [modelo de datos](architecture/03-document-model.md) → [calidad y spikes](architecture/04-quality-and-spikes.md) → [ADRs](architecture/adr/README.md).

**Diseño:** [sistema visual y comportamiento](design/01-design-system.md), con tokens, distribución, densidad, estados, atajos y accesibilidad.

**Planificación:** [entrada al plan](planning/README.md). Las clasificaciones `core`, `pro`, `advanced`, `optional` y `legacy` describen complejidad/uso; **no son ediciones de pago ni fases comprometidas**.

**Auditoría:** [inventario de nodos](catalog/node-reference.json).

## Cómo recuperar contexto con IA

| Necesidad | Leer |
|---|---|
| Una capacidad | `python3 scripts/catalog.py show RIG-005` |
| Un área | `python3 scripts/catalog.py domain DEF` y ficha del dominio |
| Encontrar un término | `python3 scripts/catalog.py search "deformación"` |
| Semántica del documento | `architecture/03-document-model.md` |
| Crear un ticket | Requisito + aceptación + dependencias del dominio |
| Revisar cobertura | Catálogo + evidencia funcional futura, nunca solo conteos |
| Nueva biblioteca | Comparativa + ADR + licencia y perfil de distribución |

Cada ficha de capacidad tiene una descripción observable y un criterio de aceptación inicial. El diseño detallado de algoritmos, todas las propiedades de cada operador y los casos límite adicionales se desarrollarán antes de implementar esa capacidad. Este análisis busca cobertura de familias y operaciones, no reproduce los manuales ni presume conocer el funcionamiento interno de un producto propietario.
