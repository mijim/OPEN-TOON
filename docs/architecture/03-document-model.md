# Modelo de documento y formato abierto

Propuesta, no formato ya implementado. Diseñar antes de dibujar pantallas evita que timeline, Xsheet y nodos acaben guardando tres escenas distintas.

## Entidades e invariantes

| Entidad | Datos mínimos | Invariante |
|---|---|---|
| Project | ID, nombre, escenas, bibliotecas, configuración | Las rutas no son identidades |
| Scene | ID, settings, capas, grafo, pistas y referencias | Una revisión describe un estado coherente |
| SceneSettings | Resolución, pixel aspect, FPS racional, rango, color | Valores positivos y límites conocidos |
| DrawingAsset | ID, versión, vector/raster, subcapas | Puede existir aunque no esté expuesto |
| ArtLayer | Tipo semántico y contenido | Orden Underlay → Colour → Line → Overlay |
| ExposureSpan | Inicio, fin exclusivo, DrawingId o vacío | Sin solapamientos ambiguos en una pista |
| AnimatableProperty | Valor base, tipo y curva opcional | Distingue reposo, valor local y evaluación |
| Keyframe | Tiempo, valor, interpolación y tangentes | Un valor por tiempo/propiedad tras resolver colisión |
| Palette/Swatch | IDs, valores o recursos, nombre | Mismo RGBA no implica misma identidad |
| Rig | Nodos, jerarquía, reposo y controles | Jerarquía acíclica y bind consistente |
| CompositorGraph | Nodos, puertos y conexiones | Tipos válidos, evaluación sin ciclos ilegales |
| AudioClip | Asset, inicio de escena, in/out, ganancia | Referencia al audio sin destrucción por recorte |
| AssetRef | Hash, tipo, metadata y procedencia | El hash identifica contenido inmutable |
| Revision | ID, parent, manifiesto y timestamp | No publica referencias a blobs inexistentes |

Un dibujo compartido por varias exposiciones es un recurso único. Quitar una celda no elimina ese recurso. Duplicar crea otro ID; clonar mantiene una relación explícita. La identidad de un color no cambia al modificar su valor, por lo que se puede recolorear toda una producción sin búsqueda por RGB.

## Tiempo

Internamente usar frame cero como origen e intervalos semiabiertos `[start, end)`. La UI puede mostrar frame uno por defecto, con conversión en un único lugar. Guardar FPS como racional reducido: por ejemplo `24000/1001`, no como `23.976` aproximado. Audio usa índices de muestra y conversión racional explícita; interpolación subframe no se redondea prematuramente.

Ejemplo: cuatro dibujos a doses ocupan `[0,2)`, `[2,4)`, `[4,6)`, `[6,8)`. Se muestran como frames 1–8, y exportar el rango produce ocho imágenes. Cambiar FPS debe ofrecer conservar conteo de frames o conservar duración, con reglas de remuestreo. Una transacción cambia todas las pistas afectadas y sus marcadores según la operación.

La reproducción usa un reloj monotónico; durante playback con sonido se sincroniza con el reloj de audio. Saltar frames para mantener fluidez es una política de preview visible, nunca una razón para omitir frames en render final.

## Geometría, raster y color

Vector: curvas y contornos conservan geometría editable, perfiles de grosor, estilos e identidad de regiones. La tessellación GPU es una caché derivada. Definir winding/fill rules, tolerancias en unidades de documento y comportamiento de auto-intersecciones. Skia u otra biblioteca no debe convertirse en el único esquema serializado de los dibujos.

Raster: tiles dispersos por capa/dibujo con alfa explícito y precisión registrada. Cambiar un tile crea una versión; áreas vacías no necesitan mapas enormes. Las texturas de pinceles tienen procedencia y licencia independientes del preset.

Composición: perfil por escena con espacio lineal de trabajo y formato flotante cuando corresponda; transformación de visualización separada. Declarar alfa premultiplicado o no premultiplicado por frontera y convertir conscientemente. No aplicar dos veces gamma ni tratar RGB oculto bajo alfa cero como color opaco. Un modo artístico que opere en espacio no lineal debe declararlo, no esconderlo.

## Contenedor propuesto

Dos representaciones: directorio de trabajo `.otoon/` y paquete de transporte `.otoonz` (nombres provisionales, pendientes de comprobar colisiones). **No son formatos de Toon Boom ni de OpenToonz.**

```text
shot.otoon/
  manifest.json       # formatVersion, projectId, sceneId, headRevision
  document.sqlite     # índices, revisiones, escenas y metadatos transaccionales
  blobs/sha256/...    # dibujos, tiles, audio y fuentes inmutables
  previews/...        # opcional, regenerable
```

La exportación de intercambio legible produce JSON versionado y recursos; sirve para diagnóstico y herramientas externas. No mantener JSON y SQLite como dos fuentes canónicas editables simultáneamente. El manifiesto indica versión y localización; el estado canónico se publica en una revisión consistente de la base y los blobs.

Un paquete de transporte se crea desde un snapshot cerrado, con inventario y hashes. No editar directamente un ZIP abierto ni meter un SQLite con WAL activo en el paquete. Utilizar backup/checkpoint y cierre controlado antes del empaquetado.

## Protocolo de guardado

1. Capturar la revisión a guardar sin bloquear durante todo el trabajo de disco.
2. Escribir blobs nuevos a archivos temporales, verificar hashes, hacer flush necesario y renombrarlos dentro del mismo volumen.
3. Confirmar la transacción de metadata/revisión que apunta a esos blobs ya durables.
4. Publicar el manifiesto mediante reemplazo atómico compatible con la plataforma; sincronizar directorio cuando corresponda.
5. Mantener la revisión anterior recuperable y limpiar temporales huérfanos en una pasada segura posterior.

La [atomicidad de SQLite](https://sqlite.org/atomiccommit.html) no hace transaccionales por sí sola los archivos externos. El formato debe admitir detectar si el manifiesto quedó atrasado respecto a la DB y resolverlo a una revisión íntegra. Nunca borrar blobs antiguos en el mismo paso que publica una revisión nueva. La recolección de basura toma como raíces todas las revisiones retenidas, exports fijados y recovery journals.

Pruebas obligatorias: interrupción en cada frontera, disco lleno, permiso denegado, recurso ausente, guardado mientras se sigue editando y doble apertura de la misma escena. Una configuración de sincronización de SQLite que sacrifica durabilidad requiere una decisión explícita y no se presenta como recuperación garantizada.

## Evolución y migración

- `formatVersion` evoluciona independientemente de la versión de aplicación.
- Migración determinista sobre una copia o nueva revisión; el original queda preservado.
- Los bloques de extensiones desconocidas se conservan y se marcan, con límites de tamaño y tipos.
- Un lector antiguo que no entiende semántica esencial abre solo lectura o rechaza; no guarda perdiendo datos silenciosamente.
- Validar límites antes de reservar memoria, dimensiones, número de elementos, rutas de ZIP, referencias y ciclos.
- Colección de fixtures de cada versión publicada y pruebas de ida/vuelta semántica.

## Formatos de intercambio

| Perfil | Objetivo propio | Pérdida o límite que debe declararse |
|---|---|---|
| PNG y secuencias | Entrega base de imágenes con alfa | Sin rigs, capas ni curvas |
| WAV/PCM | Audio de referencia y mezcla | Sin representación del documento de animación |
| SVG | Subconjunto vectorial interoperable | Filtros, fuentes y estilos complejos pueden requerir conversión |
| PSD | Capas de layout y pintura | Modos, máscaras, smart objects y efectos no siempre trasladables |
| EXR | Composición float y pases | Convenciones de canales, premultiplicación y color obligatorias |
| Vídeo | Preview y entrega | Disponibilidad de codecs y alfa depende del perfil/build |
| OTIO | Cortes, tracks, timing y referencias | No equivale a escena gráfica ni contiene el rig |
| glTF/GLB | Candidato abierto para 3D propio | No asumir soporte equivalente a FBX/Alembic de Harmony |
| FBX/Alembic/USD | Investigación de interoperabilidad profesional | Evaluar lector, licencia, animación, materiales y ejes por separado |
| Harmony TVG/XStage/TBG | Sin compatibilidad comprometida | No afirmar round-trip sin especificación y fixtures legítimos |

Cada adapter retorna un informe con elementos conservados, horneados, omitidos y errores. Una importación aceptada mantiene el original como asset o referencia de procedencia según política. No usar `.svg` o `.json` como promesa de que todo el comportamiento de otra aplicación cabe en ese formato.
