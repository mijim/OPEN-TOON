# OUT — Preview, render y exportación

[Volver al catálogo](../README.md)

> Vista generada desde `features.json` y `domains.json`; no editar a mano.

**Flujo:** Previsualizar → fijar rango y calidad → renderizar → verificar.

**Módulo:** `render`.

**Entidades:** RenderJob, RenderProfile, FrameBuffer, OutputSpec.

**Relaciones:** NOD, CAM, AUD.

**Riesgo principal:** Diferencias de preview y salida, fallos de encoder y agotamiento de memoria.

## Contrato común

Las mutaciones deben respetar transacciones, undo/redo y persistencia. Las vistas de ayuda no se exportan. Errores, cancelación y datos no soportados deben conservar el último estado válido. Estas son condiciones de OPEN-TOON que se concretarán por operación al implementar.

## OUT-001 — Playback interactivo

Reproducir a FPS de escena con rango, bucle y estado de rendimiento.

**Aceptación inicial:** El contador distingue reproducción completa de frames omitidos.

**Alcance:** `base` · **Nivel:** `core` · **Estado:** `not_started`.

**Evidencia:** `proposal`.

## OUT-002 — Preview con calidad controlada

Separar preview rápido y evaluación final mostrando simplificaciones activas.

**Aceptación inicial:** El usuario puede identificar que un efecto está omitido en preview.

**Alcance:** `base` · **Nivel:** `pro` · **Estado:** `not_started`.

**Evidencia:** `proposal`.

## OUT-003 — Timeout de preview

Limitar trabajo interactivo y señalar resultados incompletos recuperables.

**Aceptación inicial:** Una escena pesada sigue aceptando cancelar o completar el frame.

**Alcance:** `base` · **Nivel:** `pro` · **Estado:** `not_started`.

**Evidencia:** `proposal`.

## OUT-004 — Secuencias de imágenes

Exportar rango, nombre, relleno numérico, resolución y alfa.

**Aceptación inicial:** Un rango inclusivo 1 a 24 genera exactamente 24 archivos.

**Alcance:** `base` · **Nivel:** `core` · **Estado:** `not_started`.

**Evidencia:** `proposal`.

## OUT-005 — Vídeo con audio

Renderizar y codificar usando perfiles de códec disponibles por plataforma.

**Aceptación inicial:** Vídeo y mezcla de audio empiezan en el mismo instante.

**Alcance:** `base` · **Nivel:** `core` · **Estado:** `not_started`.

**Evidencia:** `proposal`.

## OUT-006 — Múltiples salidas

Configurar varias salidas con resolución y formato independientes.

**Aceptación inicial:** Una escena produce dos salidas sin cambiar parámetros de cámara globales.

**Alcance:** `base` · **Nivel:** `pro` · **Estado:** `not_started`.

**Evidencia:** `proposal`.

## OUT-007 — Batch render

Ejecutar trabajos fuera de la UI con logs, código de salida y posibilidad de reintento.

**Aceptación inicial:** Reintentar un frame fallido no vuelve a escribir frames válidos sin necesidad.

**Alcance:** `base` · **Nivel:** `pro` · **Estado:** `not_started`.

**Evidencia:** `proposal`.

## OUT-008 — Crop y escala de salida

Recortar y redimensionar en nodos de salida con filtro definido.

**Aceptación inicial:** El crop conserva el origen y dimensiones indicadas.

**Alcance:** `base` · **Nivel:** `pro` · **Estado:** `not_started`.

**Evidencia:** `proposal`.

## OUT-009 — Layout PSD y capas

Exportar layouts conservando las capas que admita el perfil.

**Aceptación inicial:** El PSD de prueba reabierto conserva registro y transparencia.

**Alcance:** `base` · **Nivel:** `pro` · **Estado:** `not_started`.

**Evidencia:** `proposal`.

## OUT-010 — Intercambio con After Effects

Documentar una exportación con datos y render por capas según perfil.

**Aceptación inicial:** El destinatario recibe un informe de efectos horneados.

**Alcance:** `base` · **Nivel:** `advanced` · **Estado:** `not_started`.

**Evidencia:** `proposal`.

## OUT-011 — GIF y salida ligera

Generar animaciones de preview con paleta, loop y limitaciones explícitas.

**Aceptación inicial:** La duración del GIF coincide dentro de la granularidad de su formato.

**Alcance:** `base` · **Nivel:** `pro` · **Estado:** `not_started`.

**Evidencia:** `proposal`.

## OUT-012 — Player de secuencias

Reproducir resultados renderizados con su audio sin abrir el editor completo.

**Aceptación inicial:** Una secuencia con frame faltante se señala claramente.

**Alcance:** `base` · **Nivel:** `pro` · **Estado:** `not_started`.

**Evidencia:** `proposal`.

## OUT-013 — Precisión y color de salida

Elegir profundidad de bits, alfa y espacio de color por salida.

**Aceptación inicial:** Un EXR float conserva valores superiores a uno en el perfil HDR.

**Alcance:** `base` · **Nivel:** `advanced` · **Estado:** `not_started`.

**Evidencia:** `proposal`.
