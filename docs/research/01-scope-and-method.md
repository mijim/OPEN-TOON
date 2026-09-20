# Alcance y metodología

## Qué vamos a construir

OPEN-TOON pretende cubrir el flujo de creación de animación de Harmony Premium: dibujo vectorial/bitmap, pintura, exposición, animación tradicional, rigging cut-out, deformación, curvas, cámara, composición, sonido, integración 3D, bibliotecas y exportación. El catálogo incluye además flujos de estudio, videojuegos y asistencia opcional para que no desaparezcan del horizonte del producto.

El usuario confirmó Harmony Premium como referencia. **No se incluye construir Storyboard Pro**, una plataforma de gestión de producción equivalente a Producer, un editor general de vídeo, un modelador 3D completo ni un servicio comercial de IA. Los puntos de intercambio con estas herramientas sí pertenecen al análisis. La [descripción oficial de Harmony](https://docs.toonboom.com/help/harmony-25/premium/about/about.html) distingue el editor, sus aplicaciones auxiliares y la integración con Storyboard Pro.

## Límites de producto

| Área | Tratamiento |
|---|---|
| Harmony Premium autónomo | Referencia base de capacidades |
| Essentials y Advanced | No recrear barreras de pago; no hace falta reproducir sus ediciones |
| Database, WebCC, Control Center | Extensión de estudio separada, con versiones y bloqueos |
| Paint, Scan y Play | Flujos especializados; su equivalencia puede vivir en espacios o herramientas auxiliares |
| Gaming SDK | Perfil futuro de exportación, no promesa de compatibilidad con el SDK propietario |
| Ember | Capacidades opcionales mediante proveedores intercambiables; no dependencia del núcleo |
| Licencias comerciales de Toon Boom | Sin equivalente de activación en OPEN-TOON |
| Formatos nativos de Harmony | Compatibilidad no demostrada; no forma parte del compromiso inicial |
| Storyboard Pro | Intercambio de recursos/timing; editor completo fuera de alcance |

## Referencia temporal

Se ha consultado la documentación Premium 25 y las notas de [25.0](https://docs.toonboom.com/help/harmony-25/premium/release-notes/harmony/harmony-25-release-notes.html), [25.1](https://docs.toonboom.com/help/harmony-25/premium/release-notes/harmony/harmony-25-1-release-notes.html) y [25.2](https://docs.toonboom.com/help/harmony-25/premium/release-notes/harmony/harmony-25-2-release-notes.html). La versión 25.2 incorpora, entre otras áreas, gestión de art layers, edición de atributos animados, operaciones sobre subnodos 3D, biblioteca de audio y límites de preview. Estas áreas están representadas en el catálogo.

Algunas URLs fallaron en el lector web y se recuperaron directamente por HTTPS desde el mismo dominio oficial. Los metadatos de recuperación y hashes están en [retrieval-log.json](retrieval-log.json). Un error de un lector no se utilizó como evidencia de ausencia de una función.

## Proceso de investigación

1. Identificar productos y edición, fijar el alcance con el usuario y consultar notas de versión.
2. Extraer las referencias de navegación de la documentación oficial. Se indexaron **1.844 URLs únicas** del árbol consultado. Se excluyen enlaces puramente de redirección; el índice no equivale a la totalidad de páginas existentes ni a 1.844 funciones distintas.
3. Clasificar las familias en **26 dominios** y redactar **282 capacidades** de OPEN-TOON, con fuentes, comportamiento y aceptación inicial.
4. Registrar por separado las entradas de referencia de nodos. Algunas son familias, no operadores, y no se suman a las capacidades como si fueran requisitos únicos.
5. Contrastar alternativas tecnológicas con documentación primaria de los proyectos.
6. Proponer arquitectura, modelo de datos, lenguaje visual, escenarios y puertas de calidad que sirvan al futuro plan.

No se redistribuyen cuerpos de manuales ni imágenes. Las descripciones de OPEN-TOON son síntesis y especificación propia, no una copia del manual. Las fuentes públicas siguen siendo la autoridad sobre la referencia comercial.

## Niveles de evidencia

| Etiqueta | Significado | No significa |
|---|---|---|
| `indexed_not_individually_reviewed` | Página localizada en la navegación oficial | Lectura completa ni requisito validado |
| `documentation_linked` | Capacidad vinculada a documentación relevante | Equivalencia exacta o prueba manual |
| `retrieved` | Página recuperada por HTTP y registrada | Que cada parámetro haya sido revisado |
| `analysis_draft` | Requisito propio propuesto y preparado para revisión | Diseño final de algoritmo |
| `not_started` | Sin implementación de aplicación | Que exista un botón funcional |

La evidencia de implementación futura deberá añadir commit, pruebas y limitaciones. No se ha ejecutado Harmony para contrastar resultados, medido su rendimiento, inspeccionado su código ni verificado todos los parámetros de cada diálogo. La exhaustividad es **de cobertura funcional por familias y operaciones documentadas**, con un índice auditable para continuar el detalle; no una certificación de paridad.

## Diferencias deliberadas

La UI tendrá identidad propia y simplificará descubrimiento de herramientas. Las decisiones de arquitectura, guardado, permisos, modelos de IA y formatos son propuestas de OPEN-TOON, aunque la operación equivalente exista en Harmony. Por ejemplo, evitar ejecución automática de scripts es una política de OPEN-TOON; el enlace a scripting describe el área de referencia, no certifica esa política en Harmony.

Los términos de referencia como Pencil, Peg o Xsheet ayudan a comparar flujos; no se copiarán recursos de marca. El nombre OPEN-TOON es provisional y conviene distinguirlo claramente de [OpenToonz](https://opentoonz.github.io/e/), un proyecto existente que también se ha considerado técnicamente.
