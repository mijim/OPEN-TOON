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

**Referencia:** [H25-f06f7a2f68](https://docs.toonboom.com/help/harmony-25/premium/reference/node/combine/blending-node.html) · `documentation_linked`.

## FX-002 — Transparencia animada

Controlar alfa independientemente de RGB.

**Aceptación inicial:** Bajar opacidad a cero produce alfa cero sin halo al recomponer.

**Alcance:** `base` · **Nivel:** `core` · **Estado:** `not_started`.

**Referencia:** [H25-b69e81e91d](https://docs.toonboom.com/help/harmony-25/premium/reference/node/filter/transparency-node.html) · `documentation_linked`.

## FX-003 — Blur gaussiano y box

Filtrar por radio con extensión de borde explícita.

**Aceptación inicial:** Un blur por tiles no deja costuras.

**Alcance:** `base` · **Nivel:** `pro` · **Estado:** `not_started`.

**Referencia:** [H25-674d718b26](https://docs.toonboom.com/help/harmony-25/premium/reference/node/filter/blur-gaussian-node.html) · `documentation_linked`.

## FX-004 — Blur direccional, radial y zoom

Aplicar desenfoques con centro, dirección y longitud animables.

**Aceptación inicial:** Mover el centro fuera del frame mantiene el comportamiento definido.

**Alcance:** `base` · **Nivel:** `pro` · **Estado:** `not_started`.

**Referencia:** [H25-83b0cb533a](https://docs.toonboom.com/help/harmony-25/premium/reference/node/filter/blur-radial-zoom-node.html) · `documentation_linked`.

## FX-005 — Bokeh y blur variable

Controlar desenfoque mediante forma y mapas espaciales.

**Aceptación inicial:** Las zonas con radio cero conservan la entrada.

**Alcance:** `base` · **Nivel:** `advanced` · **Estado:** `not_started`.

**Referencia:** [H25-e2dd78046b](https://docs.toonboom.com/help/harmony-25/premium/reference/node/filter/bokeh-blur-node.html) · `documentation_linked`.

## FX-006 — Glow y bloom

Generar halo luminoso preservando HDR cuando esté habilitado.

**Aceptación inicial:** Una fuente HDR no se recorta antes del filtro en modo float.

**Alcance:** `base` · **Nivel:** `pro` · **Estado:** `not_started`.

**Referencia:** [H25-54340ff712](https://docs.toonboom.com/help/harmony-25/premium/reference/node/filter/bloom-node.html) · `documentation_linked`.

## FX-007 — Sombras, tonos y highlights

Crear sombreado 2D por silueta y matte.

**Aceptación inicial:** La sombra puede desplazarse sin mover el dibujo fuente.

**Alcance:** `base` · **Nivel:** `pro` · **Estado:** `not_started`.

**Referencia:** [H25-92aba889fe](https://docs.toonboom.com/help/harmony-25/premium/reference/node/filter/shadow-node.html) · `documentation_linked`.

## FX-008 — Curvas y niveles de color

Modificar canales mediante curvas y rangos animables.

**Aceptación inicial:** Una curva identidad devuelve la entrada dentro de tolerancia.

**Alcance:** `base` · **Nivel:** `pro` · **Estado:** `not_started`.

**Referencia:** [H25-2abc9c921f](https://docs.toonboom.com/help/harmony-25/premium/effects/adjust-colour-curve.html) · `documentation_linked`.

## FX-009 — Color override

Sustituir muestras y texturas por identidad en ramas concretas.

**Aceptación inicial:** El mismo dibujo puede renderizarse con dos variantes simultáneas.

**Alcance:** `base` · **Nivel:** `pro` · **Estado:** `not_started`.

**Referencia:** [H25-8c75ad0708](https://docs.toonboom.com/help/harmony-25/premium/effects/about-colour-override.html) · `documentation_linked`.

## FX-010 — Fades de paleta

Interpolar colores de una paleta o muestra en el tiempo.

**Aceptación inicial:** El fade no cambia la identidad de la muestra.

**Alcance:** `base` · **Nivel:** `pro` · **Estado:** `not_started`.

**Referencia:** [H25-c4f81a63a2](https://docs.toonboom.com/help/harmony-25/premium/effects/about-colour-fade.html) · `documentation_linked`.

## FX-011 — Transformación de textura

Animar coordenadas de relleno sin mover geometría.

**Aceptación inicial:** Una textura se desplaza dentro de un contorno inmóvil.

**Alcance:** `base` · **Nivel:** `pro` · **Estado:** `not_started`.

**Referencia:** [H25-cc348d1a1d](https://docs.toonboom.com/help/harmony-25/premium/effects/modify-texture-fills-transformation.html) · `documentation_linked`.

## FX-012 — Texturas por secuencia

Sustituir una textura por frames de una secuencia externa.

**Aceptación inicial:** La selección temporal de textura respeta FPS y rango definidos.

**Alcance:** `base` · **Nivel:** `advanced` · **Estado:** `not_started`.

**Referencia:** [H25-c84b74df9e](https://docs.toonboom.com/help/harmony-25/premium/effects/override-colour-bitmap-image-sequence.html) · `documentation_linked`.

## FX-013 — Matte animado

Editar contornos animables con suavizado interior y exterior.

**Aceptación inicial:** La máscara mantiene continuidad al interpolar controles compatibles.

**Alcance:** `base` · **Nivel:** `advanced` · **Estado:** `not_started`.

**Referencia:** [H25-e329520013](https://docs.toonboom.com/help/harmony-25/premium/effects/animated-matte-generator/about-animated-matte-generator.html) · `documentation_linked`.

## FX-014 — Canales y keying

Seleccionar, intercambiar y extraer mattes desde canales de imagen.

**Aceptación inicial:** El canal seleccionado coincide con los valores originales del test.

**Alcance:** `base` · **Nivel:** `pro` · **Estado:** `not_started`.

**Referencia:** [H25-0a2a946319](https://docs.toonboom.com/help/harmony-25/premium/reference/node/filter/channel-selector-node.html) · `documentation_linked`.

## FX-015 — Generadores

Crear color plano, gradientes, rejillas y ruido como fuentes de imagen.

**Aceptación inicial:** Un generador con semilla fija es reproducible.

**Alcance:** `base` · **Nivel:** `pro` · **Estado:** `not_started`.

**Referencia:** [H25-9b7bf3ee52](https://docs.toonboom.com/help/harmony-25/premium/reference/node/generator/generator-node.html) · `documentation_linked`.

## FX-016 — Distorsión y turbulencia

Deformar coordenadas de muestreo con mapas y parámetros.

**Aceptación inicial:** Una distorsión identidad produce la entrada original.

**Alcance:** `base` · **Nivel:** `advanced` · **Estado:** `not_started`.

**Referencia:** [H25-1a92cef5c5](https://docs.toonboom.com/help/harmony-25/premium/reference/node/deformation/turbulence-node.html) · `documentation_linked`.

## FX-017 — Nitidez y antiflicker

Ofrecer filtros de detalle y estabilización visual con rango acotado.

**Aceptación inicial:** El filtro desactivado no modifica los píxeles.

**Alcance:** `base` · **Nivel:** `pro` · **Estado:** `not_started`.

**Referencia:** [H25-24b0a7913c](https://docs.toonboom.com/help/harmony-25/premium/effects/unsharp-mask.html) · `documentation_linked`.

## FX-018 — Light shading y normales

Iluminar arte mediante normales o volúmenes auxiliares.

**Aceptación inicial:** Mover una luz cambia el sombreado sin modificar el dibujo fuente.

**Alcance:** `base` · **Nivel:** `advanced` · **Estado:** `not_started`.

**Referencia:** [H25-8693901f0a](https://docs.toonboom.com/help/harmony-25/premium/effects/about-light-shading.html) · `documentation_linked`.

## FX-019 — Surface shading y sombras proyectadas

Definir superficies y elevación para sombras y oclusión.

**Aceptación inicial:** Dos superficies de distinta altura producen la relación de sombra prevista.

**Alcance:** `base` · **Nivel:** `advanced` · **Estado:** `not_started`.

**Referencia:** [H25-a9272ca17c](https://docs.toonboom.com/help/harmony-25/premium/effects/cast-shadow/about-surface-shading.html) · `documentation_linked`.

## FX-020 — OpenFX

Alojar un subconjunto declarado del estándar con plugins compatibles.

**Aceptación inicial:** Un plugin incompatible se rechaza y queda identificado en el proyecto.

**Alcance:** `base` · **Nivel:** `advanced` · **Estado:** `not_started`.

**Referencia:** [H25-dbd6ebac99](https://docs.toonboom.com/help/harmony-25/premium/effects/about-openfx.html) · `documentation_linked`.
