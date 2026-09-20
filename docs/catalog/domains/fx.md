# FX — Efectos, composición avanzada y sombreado

[Volver al catálogo](../README.md)

> Vista generada desde `features.json` y `domains.json`; no editar a mano.

**Flujo:** Añadir efecto → delimitar → animar parámetros → comprobar salida.

**Módulo:** `effects`.

**Entidades:** EffectDefinition, AnimatedParameter, Matte, SurfaceMap.

**Relaciones:** NOD, COL.

**Riesgo principal:** Precisión de color, alfa, bordes de tile y coste acumulado.

## Contrato común

Las mutaciones deben respetar transacciones, undo/redo y persistencia. Las vistas de ayuda no se exportan. Errores, cancelación y datos no soportados deben conservar el último estado válido. Estas son condiciones de OPEN-TOON que se concretarán por operación al implementar.

## FX-001 — Modos de mezcla

Combinar imágenes con ecuaciones de mezcla y alfa documentadas.

**Aceptación inicial:** Una batería de patches coincide con las ecuaciones seleccionadas.

**Alcance:** `base` · **Nivel:** `pro` · **Estado:** `not_started`.

**Evidencia:** `proposal`.

## FX-002 — Transparencia animada

Controlar alfa independientemente de RGB.

**Aceptación inicial:** Bajar opacidad a cero produce alfa cero sin halo al recomponer.

**Alcance:** `base` · **Nivel:** `core` · **Estado:** `not_started`.

**Evidencia:** `proposal`.

## FX-003 — Blur gaussiano y box

Filtrar por radio con extensión de borde explícita.

**Aceptación inicial:** Un blur por tiles no deja costuras.

**Alcance:** `base` · **Nivel:** `pro` · **Estado:** `not_started`.

**Evidencia:** `proposal`.

## FX-004 — Blur direccional, radial y zoom

Aplicar desenfoques con centro, dirección y longitud animables.

**Aceptación inicial:** Mover el centro fuera del frame mantiene el comportamiento definido.

**Alcance:** `base` · **Nivel:** `pro` · **Estado:** `not_started`.

**Evidencia:** `proposal`.

## FX-005 — Bokeh y blur variable

Controlar desenfoque mediante forma y mapas espaciales.

**Aceptación inicial:** Las zonas con radio cero conservan la entrada.

**Alcance:** `base` · **Nivel:** `advanced` · **Estado:** `not_started`.

**Evidencia:** `proposal`.

## FX-006 — Glow y bloom

Generar halo luminoso preservando HDR cuando esté habilitado.

**Aceptación inicial:** Una fuente HDR no se recorta antes del filtro en modo float.

**Alcance:** `base` · **Nivel:** `pro` · **Estado:** `not_started`.

**Evidencia:** `proposal`.

## FX-007 — Sombras, tonos y highlights

Crear sombreado 2D por silueta y matte.

**Aceptación inicial:** La sombra puede desplazarse sin mover el dibujo fuente.

**Alcance:** `base` · **Nivel:** `pro` · **Estado:** `not_started`.

**Evidencia:** `proposal`.

## FX-008 — Curvas y niveles de color

Modificar canales mediante curvas y rangos animables.

**Aceptación inicial:** Una curva identidad devuelve la entrada dentro de tolerancia.

**Alcance:** `base` · **Nivel:** `pro` · **Estado:** `not_started`.

**Evidencia:** `proposal`.

## FX-009 — Color override

Sustituir muestras y texturas por identidad en ramas concretas.

**Aceptación inicial:** El mismo dibujo puede renderizarse con dos variantes simultáneas.

**Alcance:** `base` · **Nivel:** `pro` · **Estado:** `not_started`.

**Evidencia:** `proposal`.

## FX-010 — Fades de paleta

Interpolar colores de una paleta o muestra en el tiempo.

**Aceptación inicial:** El fade no cambia la identidad de la muestra.

**Alcance:** `base` · **Nivel:** `pro` · **Estado:** `not_started`.

**Evidencia:** `proposal`.

## FX-011 — Transformación de textura

Animar coordenadas de relleno sin mover geometría.

**Aceptación inicial:** Una textura se desplaza dentro de un contorno inmóvil.

**Alcance:** `base` · **Nivel:** `pro` · **Estado:** `not_started`.

**Evidencia:** `proposal`.

## FX-012 — Texturas por secuencia

Sustituir una textura por frames de una secuencia externa.

**Aceptación inicial:** La selección temporal de textura respeta FPS y rango definidos.

**Alcance:** `base` · **Nivel:** `advanced` · **Estado:** `not_started`.

**Evidencia:** `proposal`.

## FX-013 — Matte animado

Editar contornos animables con suavizado interior y exterior.

**Aceptación inicial:** La máscara mantiene continuidad al interpolar controles compatibles.

**Alcance:** `base` · **Nivel:** `advanced` · **Estado:** `not_started`.

**Evidencia:** `proposal`.

## FX-014 — Canales y keying

Seleccionar, intercambiar y extraer mattes desde canales de imagen.

**Aceptación inicial:** El canal seleccionado coincide con los valores originales del test.

**Alcance:** `base` · **Nivel:** `pro` · **Estado:** `not_started`.

**Evidencia:** `proposal`.

## FX-015 — Generadores

Crear color plano, gradientes, rejillas y ruido como fuentes de imagen.

**Aceptación inicial:** Un generador con semilla fija es reproducible.

**Alcance:** `base` · **Nivel:** `pro` · **Estado:** `not_started`.

**Evidencia:** `proposal`.

## FX-016 — Distorsión y turbulencia

Deformar coordenadas de muestreo con mapas y parámetros.

**Aceptación inicial:** Una distorsión identidad produce la entrada original.

**Alcance:** `base` · **Nivel:** `advanced` · **Estado:** `not_started`.

**Evidencia:** `proposal`.

## FX-017 — Nitidez y antiflicker

Ofrecer filtros de detalle y estabilización visual con rango acotado.

**Aceptación inicial:** El filtro desactivado no modifica los píxeles.

**Alcance:** `base` · **Nivel:** `pro` · **Estado:** `not_started`.

**Evidencia:** `proposal`.

## FX-018 — Light shading y normales

Iluminar arte mediante normales o volúmenes auxiliares.

**Aceptación inicial:** Mover una luz cambia el sombreado sin modificar el dibujo fuente.

**Alcance:** `base` · **Nivel:** `advanced` · **Estado:** `not_started`.

**Evidencia:** `proposal`.

## FX-019 — Surface shading y sombras proyectadas

Definir superficies y elevación para sombras y oclusión.

**Aceptación inicial:** Dos superficies de distinta altura producen la relación de sombra prevista.

**Alcance:** `base` · **Nivel:** `advanced` · **Estado:** `not_started`.

**Evidencia:** `proposal`.

## FX-020 — OpenFX

Alojar un subconjunto declarado del estándar con plugins compatibles.

**Aceptación inicial:** Un plugin incompatible se rechaza y queda identificado en el proyecto.

**Alcance:** `base` · **Nivel:** `advanced` · **Estado:** `not_started`.

**Evidencia:** `proposal`.
