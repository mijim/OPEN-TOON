#!/usr/bin/env python3
"""Query the analysis catalog or regenerate its Markdown views (stdlib only)."""
from __future__ import annotations
import argparse
import json
from pathlib import Path
import unicodedata

ROOT = Path(__file__).resolve().parents[1]
CATALOG = ROOT / 'docs/catalog'


def read(name: str) -> dict:
    return json.loads((CATALOG / name).read_text(encoding='utf-8'))


def normalize(value: str) -> str:
    return ''.join(c for c in unicodedata.normalize('NFKD', value.casefold()) if not unicodedata.combining(c))


def documents() -> dict[Path, str]:
    """Return deterministic generated views without changing files."""
    features = read('features.json')['features']
    domains = read('domains.json')['domains']
    nodes = read('node-reference.json')['entries']
    files = {}
    index = ['# Catálogo funcional', '',
             f'{len(features)} capacidades originales propuestas en {len(domains)} dominios. Estado de aplicación: **sin implementar**.', '',
             'Fuente canónica: [features.json](features.json). Los criterios de aceptación son objetivos de OPEN-TOON. Las dependencias entre dominios indican relaciones, no que deba completarse un dominio entero antes de comenzar otro.', '',
             'Niveles: `core` fundamentos; `pro` flujo profesional; `advanced` alta complejidad; `optional` optativo; `legacy` compatibilidad histórica. No son fases de ejecución.', '',
             '| Dominio | Capacidades | Módulo |', '|---|---:|---|']
    for d in domains:
        selected = [f for f in features if f['domain'] == d['id']]
        filename = f"{d['id'].lower()}.md"
        index.append(f"| [{d['id']} — {d['title']}](domains/{filename}) | {len(selected)} | `{d['owner_module']}` |")
        lines = [f"# {d['id']} — {d['title']}", '', '[Volver al catálogo](../README.md)', '',
                 '> Vista generada desde `features.json` y `domains.json`; no editar a mano.', '',
                 f"**Flujo:** {d['workflow']}", '', f"**Módulo:** `{d['owner_module']}`.", '',
                 f"**Entidades:** {', '.join(d['entities'])}.", '',
                 f"**Relaciones:** {', '.join(d['depends_on']) or 'Ninguna'}.", '',
                 f"**Riesgo principal:** {d['risk']}", '',
                 '## Contrato común', '',
                 'Las mutaciones deben respetar transacciones, undo/redo y persistencia. Las vistas de ayuda no se exportan. Errores, cancelación y datos no soportados deben conservar el último estado válido. Estas son condiciones de OPEN-TOON que se concretarán por operación al implementar.', '']
        for f in selected:
            lines += [f"## {f['id']} — {f['title']}", '', f['requirement'], '',
                      f"**Aceptación inicial:** {f['acceptance_criteria'][0]}", '',
                      f"**Alcance:** `{f['scope']}` · **Nivel:** `{f['capability_level']}` · **Estado:** `{f['implementation_status']}`.", '',
                      f"**Evidencia:** `{f['evidence']}`.", '']
        files[CATALOG / 'domains' / filename] = '\n'.join(lines)
    index += ['', '## Catálogos complementarios', '',
              '- [Esquema JSON de las capacidades](features.schema.json).',
              '- [Requisitos no funcionales](nonfunctional.json).',
              '- [Inventario de nodos por categoría](nodes.md).',
              '- [Flujos integrados para aceptación](../research/02-workflows.md).', '',
              'No sumar funciones, páginas y nodos: contienen solapamientos y niveles de granularidad distintos.', '']
    files[CATALOG / 'README.md'] = '\n'.join(index)
    lines = ['# Inventario de nodos', '',
             f'{len(nodes)} entradas de operadores y familias propuestas. No se presentan como ese número de efectos distintos. Todas están pendientes de especificación y parámetros de OPEN-TOON.', '',
             'Antes de implementar cada operador: definir puertos, tipos, parámetros/unidades, valores por defecto, animabilidad, espacio de color, alfa, bounds/halo de tiles, ROI, invalidación, determinismo, errores, perfiles soportados y una escena de referencia.', '',
             'El núcleo mínimo de composición y las principales familias ya están descritos en NOD, FX, PAR, DEF, CTL y THR. Este inventario evita perder los operadores menos frecuentes durante el plan largo.', '']
    for cat in sorted({n['category'] for n in nodes}):
        lines += [f'## {cat}', '', '| ID estable | Nodo |', '|---|---|']
        lines += [f"| `{n['id']}` | {n['name']} |" for n in nodes if n['category'] == cat]
        lines += ['']
    files[CATALOG / 'nodes.md'] = '\n'.join(lines)
    return files


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    sub = parser.add_subparsers(dest='command', required=True)
    sub.add_parser('stats')
    sub.add_parser('render')
    for name, argument in [('show', 'id'), ('search', 'query'), ('domain', 'domain')]:
        sub.add_parser(name).add_argument(argument)
    args = parser.parse_args()
    features = read('features.json')['features']
    if args.command == 'render':
        files = documents()
        for path, text in files.items():
            path.parent.mkdir(parents=True, exist_ok=True)
            path.write_text(text, encoding='utf-8')
        print(f'Generated {len(files)} Markdown files.')
        return
    if args.command == 'stats':
        print(json.dumps({'features': len(features), 'domains': len(read('domains.json')['domains']),
                          'node_reference_entries': len(read('node-reference.json')['entries']),
                          'by_status': {k: sum(f['implementation_status'] == k for f in features) for k in sorted({f['implementation_status'] for f in features})}}, indent=2))
        return
    if args.command == 'show':
        selected = [f for f in features if f['id'] == args.id.upper()]
    elif args.command == 'domain':
        selected = [f for f in features if f['domain'] == args.domain.upper()]
    else:
        query = normalize(args.query)
        selected = [f for f in features if query in normalize(json.dumps(f, ensure_ascii=False))]
    print(json.dumps(selected, ensure_ascii=False, indent=2))
    if not selected:
        parser.exit(1, 'No matching feature.\n')


if __name__ == '__main__':
    main()
