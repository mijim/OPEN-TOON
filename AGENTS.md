# Instrucciones para trabajar en OPEN-TOON

## Contexto vigente

- El usuario eligió **Harmony Premium como base**. Storyboard Pro es un producto separado, fuera del alcance base.
- Se solicitó primero investigación detallada y clasificada; después se elaborará un plan largo por etapas. No iniciar una implementación completa solo porque exista este catálogo.
- La interfaz debe ser minimalista, en blanco, negro y grises, muy próxima en lenguaje visual a Vercel/Geist, con identidad propia.
- El repositorio debe ser público en la cuenta GitHub `mijim`.

## Lectura y fuentes

1. Leer `docs/INDEX.md` y el documento pertinente al módulo.
2. Consultar `docs/catalog/features.json` por ID o dominio usando `scripts/catalog.py`.
3. Consultar la fuente concreta antes de implementar. `documentation_linked` no significa prueba manual de Harmony, equivalencia exacta ni función implementada.
4. `docs/research/reference-index.json` es un índice de navegación, no 1.844 requisitos validados. `retrieval-log.json` registra descargas HTTP, no pruebas funcionales.
5. Los requisitos y criterios de aceptación de OPEN-TOON son propuestas propias. No atribuir a Harmony decisiones internas de arquitectura, rendimiento o formato.

## Mantener la documentación

- `features.json`, `domains.json`, `node-reference.json` y `nonfunctional.json` son fuentes canónicas; no renumerar IDs existentes.
- Regenerar las fichas con `python3 scripts/catalog.py render` y validar con `python3 scripts/validate_docs.py`.
- Una nueva capacidad necesita alcance, fuente o etiqueta de propuesta, comportamiento observable, aceptación y módulo propietario.
- Cambios de tecnología, serialización, semántica temporal, color o plugins necesitan actualizar el ADR correspondiente.
- Documentar implementación real por separado. Nunca marcar una capacidad terminada por crear un botón, un mockup o un test que no ejercita su comportamiento.

## Arquitectura prevista

- Núcleo C++20 sin dependencias de QML, widgets, filesystem, red ni SDKs propietarios.
- UI → aplicación → dominio; adapters implementan los puertos del dominio/aplicación.
- Toda mutación del documento pasa por comandos transaccionales y undo/redo. Vista, selección y hover tienen estado propio.
- Un único modelo de tiempo para timeline, Xsheet, audio, curvas y render. Tasas racionales, límites definidos, IDs estables.
- No tratar un nodo gráfico de Qt como un nodo del documento ni el scene graph de UI como el compositor de animación.
- No introducir servicios remotos obligatorios, microservicios, una reescritura en otra tecnología o un segundo motor sin una decisión documentada.
- Preferir módulos pequeños por capacidad con API explícita, RAII, ownership visible y validación en límites. Evitar abstracciones sin consumidor real.

## Verificación y publicación

- Probar invariantes, persistencia, undo, conversión de formatos, imágenes de referencia y escenarios de animador según el cambio.
- No descargar ni subir proyectos privados de terceros como fixtures. Usar escenas sintéticas o recursos redistribuibles con procedencia.
- No incluir credenciales, rutas personales, cachés de investigación ni manuales descargados en commits.
- GPL-3.0-or-later para aportaciones originales; revisar por dependencia módulos, plugins y binarios realmente distribuidos.
- Ramas de trabajo nuevas con prefijo `codex/`, salvo instrucción expresa del usuario.
