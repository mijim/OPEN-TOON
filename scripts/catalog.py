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
    index = ['# Functional catalog', '',
             f'{len(features)} original proposed capabilities across {len(domains)} domains. Application status: **experimental, with partial working subsets; no phase is complete**.', '',
             'Canonical source: [features.json](features.json). Acceptance criteria are OPEN-TOON objectives. Domain dependencies express relationships, not a requirement to finish an entire domain before starting another.', '',
             'Levels: `core` foundations; `pro` professional workflow; `advanced` high complexity; `optional` optional extension; `legacy` historical compatibility. These are not execution phases.', '',
             '| Domain | Capabilities | Module |', '|---|---:|---|']
    for d in domains:
        selected = [f for f in features if f['domain'] == d['id']]
        filename = f"{d['id'].lower()}.md"
        index.append(f"| [{d['id']} — {d['title']}](domains/{filename}) | {len(selected)} | `{d['owner_module']}` |")
        lines = [f"# {d['id']} — {d['title']}", '', '[Back to catalog](../README.md)', '',
                 '> Generated from `features.json` and `domains.json`; do not edit manually.', '',
                 f"**Workflow:** {d['workflow']}", '', f"**Module:** `{d['owner_module']}`.", '',
                 f"**Entities:** {', '.join(d['entities'])}.", '',
                 f"**Relationships:** {', '.join(d['depends_on']) or 'None'}.", '',
                 f"**Main risk:** {d['risk']}", '',
                 '## Shared contract', '',
                 'Mutations must respect transactions, undo/redo and persistence. Visual aids are not exported. Errors, cancellation and unsupported data must preserve the last valid state. These OPEN-TOON conditions will be specified per operation during implementation.', '']
        for f in selected:
            lines += [f"## {f['id']} — {f['title']}", '', f['requirement'], '',
                      f"**Initial acceptance:** {f['acceptance_criteria'][0]}", '',
                      f"**Scope:** `{f['scope']}` · **Level:** `{f['capability_level']}` · **Status:** `{f['implementation_status']}`.", '',
                      f"**Evidence:** `{f['evidence']}`.", '']
        files[CATALOG / 'domains' / filename] = '\n'.join(lines)
    index += ['', '## Related catalogs', '',
              '- [Capability JSON schema](features.schema.json).',
              '- [Nonfunctional requirements](nonfunctional.json).',
              '- [Node inventory by category](nodes.md).',
              '- [Integrated acceptance workflows](../research/02-workflows.md).', '',
              'Do not add capability, page and node counts: they overlap and have different levels of detail.', '']
    files[CATALOG / 'README.md'] = '\n'.join(index)
    lines = ['# Node inventory', '',
             f'{len(nodes)} proposed operator and family entries. This is not a count of distinct effects. All await OPEN-TOON specifications and parameters.', '',
             'Before implementing an operator, define ports, types, parameter units/defaults, animability, color space, alpha, tile bounds/halo, ROI, invalidation, determinism, errors, supported profiles and a reference scene.', '',
             'The minimal compositor and main families are described in NOD, FX, PAR, DEF, CTL and THR. This inventory keeps less frequent operators visible throughout the long-term plan.', '']
    for cat in sorted({n['category'] for n in nodes}):
        lines += [f'## {cat}', '', '| Stable ID | Node |', '|---|---|']
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
