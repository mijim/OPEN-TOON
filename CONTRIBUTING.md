# Contribuir

El proyecto está en investigación. Las contribuciones útiles ahora son correcciones de requisitos, fuentes primarias, fixtures redistribuibles y pruebas de viabilidad propuestas. No hay un SDK de plugins ni una API estable.

Antes de cambiar el alcance, leer [AGENTS.md](AGENTS.md). Describir el problema, el ID de capacidad, el comportamiento esperado, la evidencia y las limitaciones. Mantener identificadores existentes y regenerar el catálogo Markdown tras editar sus datos.

```sh
python3 scripts/catalog.py render
python3 scripts/validate_docs.py
```

Las aportaciones originales se ofrecen bajo GPL-3.0-or-later. Identificar la procedencia y licencia de cualquier recurso externo; no copiar manuales, assets comerciales ni escenas de clientes. Para código futuro: APIs pequeñas, cambios verificables y pruebas de comportamiento proporcionales al cambio.
