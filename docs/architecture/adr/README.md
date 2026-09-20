# Registro de decisiones de arquitectura

Los ADR propuestos deben revisarse con evidencia antes de convertirse en compromisos de implementación.

| ID | Decisión | Estado | Reabrir si |
|---|---|---|---|
| ADR-001 | Desktop C++20 + Qt Quick; dominio sin Qt | Propuesto | SP-00/01/02 demuestra alternativa claramente mejor |
| ADR-002 | Una aplicación modular, adapters y CLI con núcleo compartido | Propuesto | Exista una necesidad real de despliegue separado |
| ADR-003 | Documento con IDs estables; dibujos separados de exposición | Propuesto | Los fixtures revelen semántica incompatible con el flujo |
| ADR-004 | SQLite + blobs inmutables, revisión transaccional y transporte empaquetado | Propuesto | SP-03 no logre recuperación o costes aceptables |
| ADR-005 | Renderer detrás de contrato; Qt RHI/Skia por validar | Abierto | Se complete SP-02 |
| ADR-006 | Offline y formato abierto; red/IA/estudio optativos | Propuesto | El usuario cambie prioridades de producto |
| ADR-007 | GPL-3.0-or-later para el repositorio inicial | Adoptado | El propietario cambie licencia antes de integrar aportaciones que lo dificulten |
| ADR-008 | No prometer compatibilidad nativa Harmony ni SDK propietario | Adoptado como límite de comunicación | Existan especificación, fixtures y pruebas legítimas |

## ADR-001 — Plataforma

Contexto: el alcance principal exige entrada de tableta, geometría, audio, archivos grandes y ecosistema de gráficos. Decisión propuesta: Qt Quick para UI y C++20 para dominio/motor. Alternativas: Qt Widgets, Tauri/Rust, Electron con motor nativo y navegador. Consecuencia: mayor disciplina de C++ y trabajo de UI especializada, con menos fronteras entre librerías profesionales. Evidencia: [comparativa](../01-technology-selection.md); revisión al terminar SP-00/01/02.

## ADR-004 — Guardado

Contexto: un único archivo legible resulta atractivo, pero reescribir todos los dibujos en cada guardado degrada el trabajo; una DB aislada tampoco garantiza persistencia de archivos externos. Decisión propuesta: datos de revisión en SQLite y blobs inmutables publicados antes del commit. Consecuencia: protocolo explícito, recolección de basura por alcanzabilidad y export legible separado. Evidencia pendiente: SP-03. Véase [formato](../03-document-model.md).

## ADR-005 — Backend gráfico

No se declara elegido un renderer definitivo. QQuickRhiItem es candidato para presentar/evaluar GPU; QRhi tiene compatibilidad limitada. Skia se evalúa para paths/rasterización y una referencia CPU permite pruebas independientes. Elegir después de medir latencia, memoria, color y coste de integración. Rechazar cualquier diseño que copie el frame completo por canales serializados a cada muestra del lápiz.

## ADR-007 — Licencia

El repositorio adopta GPL-3.0-or-later para las contribuciones originales. Prioriza mantener abiertas las modificaciones redistribuidas y permite distribución de una alternativa libre. No cambia las licencias de dependencias, fuentes ni obras de usuarios. Los módulos Qt, FFmpeg y plugins se evaluarán por build concreto. Referencias en [fuentes](../../research/SOURCES.md).
