# Mapa del análisis

Fecha de corte: **20 de septiembre de 2026**. Referencia funcional: **Harmony Premium 25.x**, incluyendo las notas públicas de **25.2, build 24991, de 25 de marzo de 2026**. Es una referencia documental fijada; no una garantía de que no exista una publicación posterior o funciones no documentadas.

## Decisiones principales

| Tema | Resultado | Estado |
|---|---|---|
| Producto de referencia | Harmony Premium; Storyboard Pro fuera de la base | Confirmado por el usuario |
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

**Auditoría:** [matriz de cobertura](research/COVERAGE.md), [fuentes técnicas y de producto](research/SOURCES.md), [índice oficial](research/reference-index.json), [registro de recuperación](research/retrieval-log.json), [referencias de nodos](catalog/node-reference.json).

## Cómo recuperar contexto con IA

| Necesidad | Leer |
|---|---|
| Una capacidad | `python3 scripts/catalog.py show RIG-005` |
| Un área | `python3 scripts/catalog.py domain DEF` y ficha del dominio |
| Encontrar un término | `python3 scripts/catalog.py search "deformación"` |
| Semántica del documento | `architecture/03-document-model.md` |
| Crear un ticket | Requisito + aceptación + fuentes + dependencias del dominio |
| Revisar paridad | Índice de referencia + evidencia funcional futura, nunca solo conteos |
| Nueva biblioteca | Comparativa + ADR + licencia y perfil de distribución |

Cada ficha de capacidad tiene una descripción observable y un criterio de aceptación inicial. El diseño detallado de algoritmos, todas las propiedades de cada operador y los casos límite adicionales se desarrollarán antes de implementar esa capacidad. Este análisis busca cobertura de familias y operaciones, no reproduce los manuales ni presume conocer el funcionamiento interno de un producto propietario.
