#!/usr/bin/env python3
"""Build registered, single-image Milo limbs from the original SVG exports."""
from __future__ import annotations

import argparse
import json
from pathlib import Path

from PIL import Image

ROOT = Path(__file__).resolve().parent


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument('--output', type=Path, default=ROOT / 'continuous-parts')
    args = parser.parse_args()
    source = json.loads((ROOT / 'parts.json').read_text(encoding='utf-8'))
    rig = json.loads((ROOT / 'continuous-rig.json').read_text(encoding='utf-8'))
    parts = {part['id']: part for part in source['parts']}
    grouped = {first: (role, spec['sources']) for role, spec in rig['limbs'].items()
               for first in spec['sources'][:1]}
    consumed = {name for spec in rig['limbs'].values() for name in spec['sources'][1:]}
    args.output.mkdir(parents=True, exist_ok=True)
    manifest = []
    for part in source['parts']:
        original = part['id']
        if original in consumed:
            continue
        role, names = grouped.get(original, (original, [original]))
        image = Image.new('RGBA', tuple(rig['canvas']))
        for name in names:
            with Image.open(ROOT / parts[name]['file']) as piece:
                if piece.size != tuple(rig['canvas']):
                    raise ValueError(f'{name} has mismatched registration')
                image.alpha_composite(piece.convert('RGBA'))
        filename = f'{len(manifest) + 1:02d}-{role}.png'
        image.save(args.output / filename, optimize=True)
        parent = part['suggested_parent']
        if parent in consumed:
            parent = next(group for group, spec in rig['limbs'].items()
                          if parent in spec['sources'])
        manifest.append({'id': role, 'file': filename, 'parent': parent,
                         'pivot_canvas_px': part['pivot_canvas_px']})
    (args.output / 'manifest.json').write_text(
        json.dumps({'schema': 1, 'name': rig['name'], 'canvas': rig['canvas'],
                    'parts': manifest}, indent=2) + '\n', encoding='utf-8')
    print(f'Built {len(manifest)} registered Milo images, including four one-piece limbs.')


if __name__ == '__main__':
    main()
