# Alcance y metodología

## Qué vamos a construir

OPEN-TOON pretende cubrir dibujo vectorial y bitmap, pintura, exposición, animación tradicional, rigging cut-out, deformación, curvas, cámara, composición, sonido, integración 3D, bibliotecas y exportación. El catálogo incluye además flujos de estudio, videojuegos y asistencia opcional.

## Límites de producto

| Área | Tratamiento |
|---|---|
| Gestión de producción | Extensión de estudio separada, con versiones y bloqueos |
| Herramientas auxiliares | Flujos especializados integrados como espacios o utilidades independientes |
| SDK de videojuegos | Perfil futuro de exportación, sin dependencia propietaria |
| Asistencia generativa | Capacidades opcionales mediante proveedores intercambiables; no dependencia del núcleo |

## Referencia temporal

El análisis se mantiene versionado junto al repositorio y debe revisarse cuando cambien las decisiones de producto o arquitectura.

## Proceso de investigación

1. Fijar el alcance del producto con el usuario.
2. Clasificar las familias en **26 dominios** y redactar **282 capacidades** de OPEN-TOON, con comportamiento y aceptación inicial.
3. Registrar por separado el inventario de nodos. Algunas entradas son familias, no operadores, y no se suman a las capacidades como si fueran requisitos únicos.
4. Contrastar alternativas tecnológicas con documentación primaria de los proyectos abiertos utilizados.
5. Proponer arquitectura, modelo de datos, lenguaje visual, escenarios y puertas de calidad que sirvan al futuro plan.

Las descripciones son especificaciones propias de OPEN-TOON.

## Niveles de evidencia

| Etiqueta | Significado | No significa |
|---|---|---|
| `proposal` | Capacidad propuesta para OPEN-TOON | Implementación o validación funcional |
| `analysis_draft` | Requisito propio propuesto y preparado para revisión | Diseño final de algoritmo |
| `not_started` | Sin implementación de aplicación | Que exista un botón funcional |


## Diferencias deliberadas


El nombre OPEN-TOON es provisional y conviene distinguirlo claramente de [OpenToonz](https://opentoonz.github.io/e/), un proyecto existente que también se ha considerado técnicamente.
