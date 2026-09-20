# Riesgos, diferencias y decisiones abiertas

## Riesgos que determinan el esfuerzo

| ID | Riesgo | Consecuencia | Respuesta y condición para cerrarlo |
|---|---|---|---|
| R-01 | Alcance de suite profesional | Muchos subsistemas y especialidades; contar funciones no estima duración | Plan por flujos completos, equipo/capacidad declarados y rangos revisables |
| R-02 | Trazo y relleno vectorial insuficientes | Una UI bonita no resulta útil al dibujar | SP-01/SP-04 y evaluación con animadores |
| R-03 | Qt RHI o integración Skia compleja | Acoplamiento a versión y diferencias de GPU | Adapter aislado, SP-02 y matriz de builds |
| R-04 | Guardado de DB y assets inconsistente | Pérdida de trabajo | Protocolo de commits, fallos inyectados y revisiones inmutables |
| R-05 | Deformación avanzada | Artefactos y algoritmos caros | Validar curva/envelope antes de weighted/shape-aware |
| R-06 | Color y alfa incoherentes | Preview y render no coinciden | Pipeline explícito y fixtures de composición |
| R-07 | Compatibilidad de formatos sobreprometida | Importación destructiva y frustración | Perfiles por formato con informes de pérdida |
| R-08 | Licencias de dependencias | Binarios difíciles de redistribuir | Revisar cada módulo/build y preservar avisos |
| R-09 | Nombre parecido a OpenToonz | Confusión de identidad y expectativas | Nombre provisional y revisión antes de una release |
| R-10 | Plugin o script inestable | Fallos, acceso indebido o datos corruptos | Sin autoejecución, límites, adapter y aislamiento según tipo |
| R-11 | Colaboración prematura | Desviar trabajo de la herramienta local | Versiones y paquetes primero; servicio de estudio separado |
| R-12 | IA dependiente de proveedor | Costes y ausencia de funcionamiento offline | Funciones optativas; editor completo sin servicio de IA |
| R-13 | Corpus documental incompleto | Operadores/variantes no considerados | Índice de referencia, actualización de fuentes y revisión humana por área |
| R-14 | Conteos interpretados como avance | Falsa sensación de producto terminado | Estado `not_started` y distinción entre capacidad, fuente y nodo |

## Decisiones resueltas o propuestas

| ID | Decisión | Estado |
|---|---|---|
| D-01 | Harmony Premium como referencia base | Confirmado por el usuario |
| D-02 | Interfaz minimalista blanco/negro, cercana a Vercel | Confirmado por el usuario |
| D-03 | GitHub público en `mijim` | Confirmado por el usuario |
| D-04 | Investigar antes del plan y construir después | Confirmado por el usuario |
| D-05 | Escritorio local; sin cuenta obligatoria | Recomendación |
| D-06 | C++20/Qt Quick | Recomendación condicionada a SP-00/01/02 |
| D-07 | Formato propio abierto; sin promesa de leer TVG/XStage/TBG | Recomendación |
| D-08 | GPL-3.0-or-later para aportaciones originales | Licencia inicial aplicada |
| D-09 | Backend gráfico exacto, dependencias y versiones | Abierto hasta los spikes |
| D-10 | Storyboard Pro completo | Fuera de base; ampliación solo si se decide expresamente |

## Información que mejorará el futuro plan

No impide completar el análisis. Antes de estimar calendario: personas disponibles, experiencia C++/Qt/gráficos, presupuesto, sistemas/tabletas reales, prioridad de animación tradicional frente a cut-out y primer flujo de producción que debe completarse. Si no se aportan estos datos, el plan deberá explicitar sus supuestos y estimar por tamaños/riesgo, no dar fechas ficticias.

El análisis propone un núcleo propio pero aún no audita código de OpenToonz. Tampoco tiene mediciones de latencia, render o compatibilidad de dispositivos. Estas son incertidumbres técnicas concretas, registradas en los spikes, no razones para volver a investigar indefinidamente antes de producir una primera herramienta útil.

## Detalle que debe cerrarse por capacidad

El catálogo cubre familias y operaciones; antes de implementar cada una se especificarán sus parámetros, valores por defecto, límites, unidades, formatos soportados, transacciones, casos de cancelación y tests. Para los nodos hay [193 entradas de referencia](../catalog/nodes.md), con páginas tanto de familia como de operador. No se ha redactado un clon de cada diálogo ni se ha afirmado conocer algoritmos internos de Harmony.

Un requisito propio puede ser más estricto que su referencia, como guardado tolerante a fallos, scripting sin autoejecución o informes de importación. Las fuentes acreditan el área funcional; esas políticas son decisiones propuestas de OPEN-TOON. En particular, la página oficial de batch render describe la cola de **Harmony Database**; el render local sin UI propuesto para OPEN-TOON no debe atribuirse a esa edición autónoma de Harmony.

## Mantenimiento de la investigación

Al actualizar la referencia de Harmony: registrar versión/fecha, revisar notas nuevas, comparar URLs del índice, asociar cambios a IDs existentes, crear IDs nuevos solo para capacidades nuevas y señalar funciones retiradas sin borrar su historial. No incrementar artificialmente el número de requisitos dividiendo botones equivalentes. La cobertura se evalúa por flujos y precisión de especificación.
