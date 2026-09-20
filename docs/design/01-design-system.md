# Diseño visual y experiencia de edición

## Dirección

Blanco, negro y grises, tipografía sobria, líneas finas, superficies planas y controles precisos. Tomar como referencia el lenguaje de [Geist](https://vercel.com/geist/stack), su [jerarquía de color](https://vercel.com/geist/colors) y su [tipografía](https://vercel.com/geist/typography). Construir componentes propios para un editor de animación; no trasladar literalmente el layout de un dashboard ni asumir que la documentación pública concede licencia sobre todos los recursos de marca.

La estética afecta a la interfaz. **El arte del usuario conserva todo su color**, y las paletas muestran colores reales. El lienzo admite fondo blanco, gris neutro o transparencia. Las ayudas del animador pueden usar colores configurables, pero por defecto emplean patrones, opacidad y símbolos legibles en monocromo.

## Tokens propuestos

Valores propios de OPEN-TOON, inspirados en la referencia; no se presentan como tokens oficiales de Vercel.

| Token | Claro | Oscuro | Uso |
|---|---|---|---|
| `surface.base` | `#FFFFFF` | `#0A0A0A` | Ventana |
| `surface.panel` | `#FAFAFA` | `#111111` | Paneles y barra |
| `surface.hover` | `#F0F0F0` | `#222222` | Hover |
| `surface.selected` | `#E8E8E8` | `#303030` | Selección acompañada de indicador |
| `text.primary` | `#171717` | `#EDEDED` | Texto principal |
| `text.secondary` | `#666666` | `#A3A3A3` | Texto secundario |
| `border.subtle` | `#E5E5E5` | `#2A2A2A` | Separadores decorativos |
| `border.control` | `#767676` | `#777777` | Límite de controles cuando sea necesario distinguirlos |
| `focus.ring` | `#171717` | `#FFFFFF` | Anillo doble con separación del fondo |
| `action.primary.bg` | `#171717` | `#EDEDED` | Acción principal |
| `action.primary.fg` | `#FFFFFF` | `#0A0A0A` | Texto de acción principal |

Los separadores decorativos no son suficientes para comunicar foco o límites esenciales. Los estados deshabilitados no se reutilizan como texto secundario legible. Errores usan icono, explicación y acción de recuperación; no se distinguen solo por rojo.

Tipografía propuesta: Geist Sans para UI y Geist Mono para timecode, frames y valores alineados. La [fuente Geist está bajo SIL OFL 1.1](https://github.com/vercel/geist-font/blob/main/OFL.txt); cuando se incluya se conservará su licencia. Se admite fallback del sistema sin red. Tamaños: 12 px para metadata secundaria, 13–14 px para controles y 16–20 px para encabezados, con escalado de UI. No usar títulos gigantes en el área de trabajo.

Espaciado base 4 px; escala 4/8/12/16/24. Radio 4–6 px en controles, sin tarjetas enormes ni sombras decorativas. Filas de 28–32 px en modo compacto y 36–40 px en modo cómodo. Objetivos interactivos de al menos 24×24 píxeles lógicos cuando corresponda, ampliables para lápiz/táctil. Los iconos pueden ser menores dentro de esa zona.

## Distribución

```text
┌ Proyecto / escena ── espacio de trabajo ── guardado ── búsqueda ── exportar ┐
├ Herramienta activa / tamaño / modo / controles contextuales ───────────────┤
│       │                                              │                  │
│ Tools │          Cámara / Drawing viewport            │ Inspector        │
│       │          lienzo como área dominante           │ Tool properties  │
│       │                                              │ Color / Library  │
├───────┴──────────────────────────────────────────────┴──────────────────┤
│ Timeline / Xsheet / Nodes        transporte · frame · rango · FPS       │
│ pistas, exposiciones o grafo; altura ajustable                           │
├ información útil: selección, coordenadas, zoom, calidad de preview ──────┤
```

Ancho inicial de herramientas 48 px; inspector entre 260 y 340 px; zona temporal aproximadamente un cuarto de altura, modificable. Son puntos de partida, no restricciones rígidas. El viewport recibe el espacio restante. No mostrar todas las herramientas de rigging, dibujo y composición simultáneamente.

| Espacio | Centro | Inferior | Inspector |
|---|---|---|---|
| Dibujo | Drawing/Camera | Timeline corta o Xsheet | Pincel, paleta, onion skin |
| Animación | Camera | Timeline y curvas | Transformación, exposición y sustituciones |
| Rig | Camera con controles | Jerarquía/nodos | Pivotes, reposo e influencias |
| Composición | Preview de cámara | Grafo de nodos | Parámetros, máscaras y salida |
| Revisión | Imagen amplia | Transporte y notas | Calidad, comparación y exportación |

Mínimo orientativo de evaluación: 1280×800 con paneles colapsables; trabajo recomendado a 1920×1080 o superior. A 200 % de escala deben seguir disponibles acciones esenciales mediante menús y colapso. Multi-monitor guarda geometría relativa y recupera ventanas fuera de pantalla. Tableta de dibujo externa no implica prometer una versión iPad/Android.

## Comportamiento de componentes

| Componente | Contrato de interacción |
|---|---|
| Botón de herramienta | Seleccionado visible por fondo y marca; tooltip con nombre y atajo |
| Campo numérico | Unidad explícita, entrada directa, arrastre opcional, reset y estado animado |
| Propiedad animada | Distingue valor base, clave actual e interpolado mediante símbolo/forma y texto |
| Timeline | Separación clara entre celda vacía, hold, nuevo dibujo, clave continua y clave sostenida |
| Xsheet | Numeración, selección rectangular, edición por teclado y encabezados fijos |
| Nodo | Nombre, tipo, puertos distinguibles por forma/etiqueta y estados de error |
| Barra de guardado | Modificado, guardando, guardado, recuperación disponible y error, sin estados ambiguos |
| Progreso | Operación, avance si es medible, cancelar y resultado; no barra ficticia |
| Diálogo de importación | Vista previa, opciones pertinentes e informe de pérdidas |
| Búsqueda de comandos | Nombres, sinónimos y atajos; no requiere servicio remoto |

Un click modifica selección; doble click entra en edición cuando sea convencional. Gestos de arrastre siempre tienen alternativa mediante campo, menú o teclado. Los scrubs numéricos muestran el valor antes de confirmar y permiten cancelar. No usar hover como único acceso a una función esencial.

## Accesibilidad y localización

Usar los criterios de [WCAG 2.2](https://www.w3.org/TR/WCAG22/) como guía verificable para contraste, foco y acciones por puntero, sin afirmar una certificación de app nativa. Objetivo: 4,5:1 para texto normal, foco distinguible y nombres/roles en la capa accesible de Qt. La geometría artística no puede reducirse por completo a controles estándar, pero sus operaciones y propiedades deben tener alternativas accesibles.

Atajos contextuales con conflictos detectados, soporte de teclado español, IME y redistribución de teclas. Strings traducibles; no concatenar mensajes que impidan pluralización. Los números y unidades se muestran según locale cuando proceda, mientras que el formato de archivo usa una representación estable e independiente del idioma.

## Validación visual futura

Escenas de UI: proyecto vacío, dibujo cargado, 300 capas, nombres largos, multi-selección, error de recurso, render parcial, modo oscuro y 200 % de escala. Comprobar densidad, ausencia de clipping, legibilidad de texto y separación de canvas/UI. Las transiciones de UI serán cortas y reducibles; nunca añadir easing visual al playhead que cambie el tiempo percibido.

La captura de producto para la futura landing deberá proceder de la aplicación real. Este documento define la dirección visual; no es una maqueta funcional ni una promesa de interfaz terminada.
