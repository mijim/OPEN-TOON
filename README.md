# OPEN-TOON

An independent open-source 2D animation application in the research and specification stage, using Toon Boom Harmony Premium as a functional reference.

**Estado: análisis y especificación. Todavía no existe una aplicación ejecutable.** No se afirma compatibilidad con archivos de Harmony ni paridad de funciones. Proyecto independiente, sin afiliación con Toon Boom, Vercel u OpenToonz.

El objetivo es reunir dibujo vectorial y bitmap, animación tradicional y cut-out, rigging, composición y herramientas de producción en una aplicación de escritorio con una interfaz limpia en blanco, negro y grises.

## Consultar el análisis

1. [Índice y resumen de decisiones](docs/INDEX.md).
2. [Alcance, metodología y límites](docs/research/01-scope-and-method.md).
3. [Catálogo funcional: 282 capacidades en 26 dominios](docs/catalog/README.md).
4. [Comparativa tecnológica y recomendación](docs/architecture/01-technology-selection.md).
5. [Arquitectura y estructura del código](docs/architecture/02-system-design.md).
6. [Modelo de datos y formato de proyecto](docs/architecture/03-document-model.md).
7. [Diseño visual y experiencia de edición](docs/design/01-design-system.md).
8. [Calidad, rendimiento y pruebas de viabilidad](docs/architecture/04-quality-and-spikes.md).
9. [Riesgos y decisiones abiertas](docs/research/03-risks-and-decisions.md).
10. [Entrada al futuro plan de implementación](docs/planning/README.md).

## Para agentes de IA

Leer [AGENTS.md](AGENTS.md), después [docs/INDEX.md](docs/INDEX.md). El catálogo canónico es [features.json](docs/catalog/features.json); las fichas Markdown son vistas generadas. Los IDs de funciones y las fuentes permiten recuperar contexto por módulo sin cargar toda la investigación.

```sh
python3 scripts/catalog.py stats
python3 scripts/catalog.py show DEF-005
python3 scripts/catalog.py search "paleta"
python3 scripts/catalog.py domain RIG
python3 scripts/catalog.py render
python3 scripts/validate_docs.py
```

Las herramientas documentales usan únicamente la biblioteca estándar de Python 3.11 o posterior. No se necesitan dependencias de la futura aplicación para consultar el análisis.

## Dirección técnica propuesta

Escritorio para Windows, macOS y Linux; **C++20 + Qt 6 / Qt Quick (QML)**, núcleo de dominio independiente de la interfaz, motor gráfico intercambiable, almacenamiento local y render sin interfaz. La elección de backend gráfico requiere pruebas antes de considerarse definitiva. La apariencia inspirada en Geist no obliga a usar una tecnología web.

El plan largo de implementación se elaborará después de revisar este análisis, respetando la secuencia solicitada. No hay fechas ni promesas de paridad publicadas.

## Licencia

Las aportaciones originales de este repositorio se distribuyen bajo **GPL-3.0-or-later**; véase [LICENSE](LICENSE). Los documentos y recursos externos enlazados conservan sus licencias. No se redistribuyen manuales, ilustraciones, código ni recursos de Toon Boom. Las obras creadas por los usuarios no adquieren la licencia de la aplicación por el mero hecho de utilizarla.

Nombre de trabajo: OPEN-TOON. Antes de una distribución pública de binarios se revisará su posible confusión con otros proyectos de animación.
