# IMP — Importación, escaneo e intercambio

[Volver al catálogo](../README.md)

> Vista generada desde `features.json` y `domains.json`; no editar a mano.

**Flujo:** Importar → verificar conversiones → enlazar recursos → corregir pérdidas.

**Módulo:** `interchange`.

**Entidades:** ImportJob, FormatAdapter, ConversionReport, ExternalAsset.

**Relaciones:** PRJ, LYR.

**Riesgo principal:** La extensión de archivo no implica fidelidad de todas sus funciones.

## Contrato común

Las mutaciones deben respetar transacciones, undo/redo y persistencia. Las vistas de ayuda no se exportan. Errores, cancelación y datos no soportados deben conservar el último estado válido. Estas son condiciones de OPEN-TOON que se concretarán por operación al implementar.

## IMP-001 — Imágenes y secuencias

Importar imágenes sueltas o secuencias con alfa, orden y exposición explícitos.

**Aceptación inicial:** Una secuencia con huecos informa cómo los resuelve.

**Alcance:** `base` · **Nivel:** `core` · **Estado:** `not_started`.

**Referencia:** [H25-e055a18f60](https://docs.toonboom.com/help/harmony-25/premium/import/import-bitmap-image.html) · `documentation_linked`.

## IMP-002 — PSD por capas

Leer jerarquía, opacidad y modos soportados, informando de pérdidas.

**Aceptación inicial:** Un modo no soportado se informa antes de aceptar la conversión.

**Alcance:** `base` · **Nivel:** `pro` · **Estado:** `not_started`.

**Referencia:** [H25-564fdb2e3a](https://docs.toonboom.com/help/harmony-25/premium/import/import-multi-layer-psd-file.html) · `documentation_linked`.

## IMP-003 — Layout PSD

Reutilizar composición y posiciones de un layout con escala conocida.

**Aceptación inicial:** Las capas conservan registro relativo tras importar.

**Alcance:** `base` · **Nivel:** `pro` · **Estado:** `not_started`.

**Referencia:** [H25-79550a6433](https://docs.toonboom.com/help/harmony-25/premium/import/import-psd-layout.html) · `documentation_linked`.

## IMP-004 — Vectores externos

Convertir formatos vectoriales compatibles preservando curvas, rellenos y transformaciones soportadas.

**Aceptación inicial:** El informe enumera filtros o fuentes no trasladables.

**Alcance:** `base` · **Nivel:** `pro` · **Estado:** `not_started`.

**Referencia:** [H25-f6a8928d43](https://docs.toonboom.com/help/harmony-25/premium/import/import-vector-files.html) · `documentation_linked`.

## IMP-005 — Vídeo de referencia

Importar vídeo para referencia o extracción de frames con reloj explícito.

**Aceptación inicial:** Un clip de tasa variable se normaliza sin deriva oculta.

**Alcance:** `base` · **Nivel:** `pro` · **Estado:** `not_started`.

**Referencia:** [H25-aa2bd5fb04](https://docs.toonboom.com/help/harmony-25/premium/import/import-quicktime-movie.html) · `documentation_linked`.

## IMP-006 — Vincular imagen externa

Conservar enlace y permitir actualizar contenido desde disco.

**Aceptación inicial:** Una actualización no borra la transformación de la capa.

**Alcance:** `base` · **Nivel:** `pro` · **Estado:** `not_started`.

**Referencia:** [H25-e967e8b467](https://docs.toonboom.com/help/harmony-25/premium/import/link-layer-external-image.html) · `documentation_linked`.

## IMP-007 — Escaneo

Adquirir dibujos con resolución, registro y orden de numeración.

**Aceptación inicial:** Reescanear una hoja puede sustituir su dibujo sin cambiar exposiciones.

**Alcance:** `base` · **Nivel:** `advanced` · **Estado:** `not_started`.

**Referencia:** [H25-59387f2abb](https://docs.toonboom.com/help/harmony-25/premium/import/about-scan.html) · `documentation_linked`.

## IMP-008 — Vectorización

Convertir escaneos a trazos o regiones con parámetros repetibles.

**Aceptación inicial:** El mismo preset y entrada producen igual salida.

**Alcance:** `base` · **Nivel:** `advanced` · **Estado:** `not_started`.

**Referencia:** [H25-3b47817346](https://docs.toonboom.com/help/harmony-25/premium/import/about-custom-vectorization-parameter.html) · `documentation_linked`.

## IMP-009 — FLA y SWF heredados

Documentar conversión asistida y límites de formatos antiguos.

**Aceptación inicial:** Una función no convertible queda en un informe, no desaparece sin aviso.

**Alcance:** `legacy_candidate` · **Nivel:** `legacy` · **Estado:** `not_started`.

**Referencia:** [H25-44a46da8d7](https://docs.toonboom.com/help/harmony-25/premium/import/about-fla-file-import.html) · `documentation_linked`.

## IMP-010 — Intercambio editorial

Recibir layouts o animatics como escenas con timing y recursos.

**Aceptación inicial:** La duración del plano importado coincide con el animatic de referencia.

**Alcance:** `base` · **Nivel:** `pro` · **Estado:** `not_started`.

**Referencia:** [H25-acfe671ce8](https://docs.toonboom.com/help/harmony-25/premium/about/about.html) · `documentation_linked`.
