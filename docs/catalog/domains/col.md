# COL — Pintura, paletas y gestión de color

[Volver al catálogo](../README.md)

> Vista generada desde `features.json` y `domains.json`; no editar a mano.

**Flujo:** Definir paleta → rellenar → corregir → generar variaciones → comprobar color.

**Módulo:** `colour`.

**Entidades:** Palette, SwatchId, Gradient, TextureFill, ColourConfig.

**Relaciones:** LYR, VEC, RAS.

**Riesgo principal:** Confundir identidad de color con valores RGB y perder consistencia de producción.

## Contrato común

Las mutaciones deben respetar transacciones, undo/redo y persistencia. Las vistas de ayuda no se exportan. Errores, cancelación y datos no soportados deben conservar el último estado válido. Estas son condiciones de OPEN-TOON que se concretarán por operación al implementar.

## COL-001 — Paletas y colores identificados

Referenciar colores por identificador estable separado de su valor RGBA.

**Aceptación inicial:** Cambiar una muestra recolorea todas sus referencias y ninguna muestra distinta.

**Alcance:** `base` · **Nivel:** `core` · **Estado:** `not_started`.

**Referencia:** [H25-6066b2d11c](https://docs.toonboom.com/help/harmony-25/premium/colour/about-palette.html) · `documentation_linked`.

## COL-002 — Crear y editar muestras

Gestionar muestras sólidas, nombres, opacidad y duplicados.

**Aceptación inicial:** Dos muestras con igual RGB pueden conservar identidades diferentes.

**Alcance:** `base` · **Nivel:** `core` · **Estado:** `not_started`.

**Referencia:** [H25-e663fb19b1](https://docs.toonboom.com/help/harmony-25/premium/colour/about-colour-swatch.html) · `documentation_linked`.

## COL-003 — Relleno por regiones

Pintar regiones cerradas con tolerancia definida.

**Aceptación inicial:** Una región vecina separada por una línea no recibe pintura.

**Alcance:** `base` · **Nivel:** `core` · **Estado:** `not_started`.

**Referencia:** [H25-8beb8028a8](https://docs.toonboom.com/help/harmony-25/premium/colour/about-paint.html) · `documentation_linked`.

## COL-004 — Pintar líneas y áreas

Permitir recolorear contornos y rellenos de manera independiente.

**Aceptación inicial:** Pintar solo líneas preserva colores interiores.

**Alcance:** `base` · **Nivel:** `core` · **Estado:** `not_started`.

**Referencia:** [H25-8beb8028a8](https://docs.toonboom.com/help/harmony-25/premium/colour/about-paint.html) · `documentation_linked`.

## COL-005 — Cierre de huecos

Resolver discontinuidades pequeñas para facilitar el relleno sin unir áreas arbitrarias.

**Aceptación inicial:** Un hueco por encima de la tolerancia sigue abierto.

**Alcance:** `base` · **Nivel:** `pro` · **Estado:** `not_started`.

**Referencia:** [H25-8beb8028a8](https://docs.toonboom.com/help/harmony-25/premium/colour/about-paint.html) · `documentation_linked`.

## COL-006 — Pintura múltiple

Aplicar operaciones de pintado a selección de dibujos o rango temporal.

**Aceptación inicial:** El resumen indica qué dibujos únicos se alteraron.

**Alcance:** `base` · **Nivel:** `pro` · **Estado:** `not_started`.

**Referencia:** [H25-8beb8028a8](https://docs.toonboom.com/help/harmony-25/premium/colour/about-paint.html) · `documentation_linked`.

## COL-007 — Gradientes y texturas

Usar rellenos graduales y texturas con transformación independiente.

**Aceptación inicial:** Rotar textura no cambia la forma que la recorta.

**Alcance:** `base` · **Nivel:** `pro` · **Estado:** `not_started`.

**Referencia:** [H25-4e01cf508a](https://docs.toonboom.com/help/harmony-25/premium/colour/about-gradient-texture-tool.html) · `documentation_linked`.

## COL-008 — Modelo de color

Comparar y muestrear una referencia de color asociada al proyecto.

**Aceptación inicial:** Cambiar de modelo no altera automáticamente el dibujo.

**Alcance:** `base` · **Nivel:** `pro` · **Estado:** `not_started`.

**Referencia:** [H25-c4369655fe](https://docs.toonboom.com/help/harmony-25/premium/colour/about-colour-model.html) · `documentation_linked`.

## COL-009 — Listas de paletas

Resolver paletas de escena, elemento y producción con orden explícito.

**Aceptación inicial:** Un conflicto de identidad se resuelve de forma reproducible.

**Alcance:** `base` · **Nivel:** `pro` · **Estado:** `not_started`.

**Referencia:** [H25-282f8d387c](https://docs.toonboom.com/help/harmony-25/premium/colour/about-palette-list-levels.html) · `documentation_linked`.

## COL-010 — Clones y variantes de paleta

Compartir identidades entre variaciones cromáticas sin recolorear a mano.

**Aceptación inicial:** Una variante nocturna afecta solo a la instancia configurada.

**Alcance:** `base` · **Nivel:** `pro` · **Estado:** `not_started`.

**Referencia:** [H25-d23d2c621d](https://docs.toonboom.com/help/harmony-25/premium/colour/about-advanced-palette-management.html) · `documentation_linked`.

## COL-011 — Importar y exportar paletas

Transferir paletas con recursos de textura y reglas de conflicto.

**Aceptación inicial:** La importación repetida no duplica silenciosamente identidades.

**Alcance:** `base` · **Nivel:** `pro` · **Estado:** `not_started`.

**Referencia:** [H25-4c265a6d0e](https://docs.toonboom.com/help/harmony-25/premium/colour/about-export-palettes.html) · `documentation_linked`.

## COL-012 — Recuperar colores

Restaurar o reasignar colores cuyos recursos no estén disponibles.

**Aceptación inicial:** Abrir sin una paleta muestra el problema sin reemplazar los IDs originales.

**Alcance:** `base` · **Nivel:** `core` · **Estado:** `not_started`.

**Referencia:** [H25-a815ad80e3](https://docs.toonboom.com/help/harmony-25/premium/colour/about-back-up-palettes.html) · `documentation_linked`.

## COL-013 — Optimizar paletas

Detectar muestras redundantes o sin uso respetando revisiones existentes.

**Aceptación inicial:** Una muestra usada en una versión histórica no se elimina por defecto.

**Alcance:** `base` · **Nivel:** `pro` · **Estado:** `not_started`.

**Referencia:** [H25-471f2a2830](https://docs.toonboom.com/help/harmony-25/premium/colour/about-optimizing-palettes.html) · `documentation_linked`.

## COL-014 — Espacios de color

Distinguir interpretación de entrada, composición, visualización y salida.

**Aceptación inicial:** Una carta de referencia completa el recorrido con error medido.

**Alcance:** `base` · **Nivel:** `pro` · **Estado:** `not_started`.

**Referencia:** [H25-e331c6c3ff](https://docs.toonboom.com/help/harmony-25/premium/colour/about-colour-space-management.html) · `documentation_linked`.

## COL-015 — Configuraciones de estudio

Permitir una configuración de color compartida y validada.

**Aceptación inicial:** Una configuración que falta produce aviso y no una conversión silenciosa.

**Alcance:** `base` · **Nivel:** `advanced` · **Estado:** `not_started`.

**Referencia:** [H25-b97f31e9f7](https://docs.toonboom.com/help/harmony-25/premium/colour/customize-colour-spaces.html) · `documentation_linked`.
