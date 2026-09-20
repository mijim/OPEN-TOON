# Calidad y pruebas de viabilidad

Los [26 requisitos no funcionales](../catalog/nonfunctional.json) son objetivos propuestos, **no resultados medidos**. Una aplicación de animación se valida con dibujos, tiempo, persistencia y dispositivos, no solamente con tests de componentes UI.

## Fixtures de carga propuestos

| Escena | Contenido sintético | Finalidad |
|---|---|---|
| B0 | 1 capa, 1 dibujo, trazos de tableta grabados, 1080p | Latencia y fidelidad de entrada |
| B1 | 100 capas, 1.000 frames, 200 dibujos únicos, 100.000 segmentos vectoriales totales, 1 pista WAV de 48 kHz, máscaras básicas | Presupuesto inicial de UI, caché y playback |
| B2 | 300 capas, 10.000 frames, 1.000 dibujos, curvas y rigs reutilizados | Virtualización e invalidación |
| B3 | 4K, raster disperso, texturas, blur, máscaras y transparencias | Memoria, tiles y calidad |
| B4 | Rig con curvas/envelope, 60 controles, sustituciones y poses | Deformación y controles |
| B5 | HDR, cartas de color, alfa, EXR y múltiples salidas | Coherencia del compositor |
| B6 | Proyectos corruptos, recursos ausentes, versiones viejas | Recuperación y parsers |

Los fixtures deberán generarse con semilla fija, guardar hashes y declarar si el conteo es de segmentos totales o visibles. El hardware de referencia se registrará antes de ejecutar pruebas: CPU, GPU, RAM/VRAM, sistema, driver, tableta, refresh, DPI, resolución y build release. Una máquina con 16 GiB es una referencia de presupuesto, no un requisito mínimo ya validado.

## Puertas técnicas

| ID | Incertidumbre | Experimento acotado | Evidencia para decidir |
|---|---|---|---|
| SP-00 | Núcleo propio frente a OpenToonz | Auditar guardado, exposición, vector/raster, Plastic y compositor en un checkout identificado; probar build y una modificación de UI | Mapa de acoplamientos, licencias y coste de reutilización frente a desarrollo propio |
| SP-01 | Entrada Qt y respuesta del lápiz | Capturar presión/inclinación y dibujar B0 en cada SO con tabletas reales; probar HiDPI, pérdida de foco y cancelación | Trace interno, vídeo para latencia física y lista de drivers; cambiar de adapter si falla |
| SP-02 | Renderer y Qt Quick | Comparar CPU+upload y GPU RHI; evaluar Skia en el límite vectorial; tiles, alfa, filtros y superficies | Frame times, memoria, tiempos de build y equivalencia visual; una única ruta principal elegida |
| SP-03 | Guardado robusto | Prototipo mínimo de SQLite+blobs con fallos inyectados entre cada paso | Recuperación sin referencias rotas y coste del guardado concurrente |
| SP-04 | Topología vectorial y pintura | Autointersecciones, líneas variables, regiones adyacentes, cierre de huecos y goma | Suite de dibujos patológicos y criterios de error; elegir algoritmo/librería |
| SP-05 | Modelo temporal y audio | Mezclar FPS enteros/racionales, clips y cambios de tasa durante 10 minutos | Cero errores de conteo y deriva dentro del objetivo |
| SP-06 | Deformación | Prototipo de curva/envelope y malla sobre B4 con textura cuadriculada | Calidad de juntas, continuidad, rendimiento y límites explícitos |
| SP-07 | Distribución libre | Construir paquete mínimo con módulos Qt, fuentes y codecs previstos | Dependencias y licencias exactas, instalación en sistema limpio y carga de plugins |

No hace falta resolver partículas o IA antes del primer producto útil. Sí hace falta conocer si la pila elegida puede dibujar, guardar y reproducir con garantías. Si un spike falla, actualizar el ADR y el plan; no esconderlo detrás de un objetivo de diseño.

## Tipos de pruebas

**Dominio:** aritmética temporal, intervalos, identidades, curvas, transformaciones, comandos inversos y orden del grafo. Pruebas generativas para secuencias de edición; comparan invariantes, no una copia del algoritmo de implementación.

**Persistencia:** guardar/abrir, migraciones, backups, disco lleno, rutas Unicode, interrupciones y acceso concurrente. El criterio es igualdad semántica y recursos recuperables, no byte a byte de una DB con timestamps.

**Imagen:** render CPU determinista cuando sea posible y comparación perceptual/numérica de GPU con tolerancias por operador. Comparar alfa por separado; incluir bordes, imágenes fuera de frame, HDR y combinaciones de paleta/deformación/máscara. Investigar cambios de golden antes de aceptarlos.

**UI:** navegación de teclado, selección, operaciones de rango, textos largos, escalado, estados vacíos y recuperación de foco. El trazado sintético sirve para regresiones de geometría; no sustituye la prueba física de tableta.

**Rendimiento:** warm/cold cache separados, p50/p95/p99, regresiones sobre la misma máquina y build, CPU/GPU/IO diferenciados. Medir guardar mientras se dibuja y evitar promedios que oculten pausas largas.

**Flujo artístico:** completar [WF-01 a WF-12](../research/02-workflows.md) con una persona animadora, registrando bloqueos, pérdidas y pasos confusos. La paridad no se obtiene por contar menús.

## Definition of Done futura

Una capacidad pasa de `not_started` a implementada cuando realiza su operación sobre el modelo real, persiste, se deshace cuando aplica, explica errores y tiene aceptación observable. Pasa a verificada cuando además supera las plataformas/perfiles declarados y documenta límites. Funciones parcialmente soportadas se marcan `partial`, con el subconjunto exacto.

Para una release: builds limpios, avisos de terceros, formato versionado, escenas de referencia, prueba de actualización/recuperación, paquete instalable y guía de limitaciones. Los objetivos de render 4K/8K, número de capas y codecs se anunciarán solo después de medirlos.
