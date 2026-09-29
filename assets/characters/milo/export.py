#!/usr/bin/env python3
"""Export Milo's SVG layers. Optional asset tools: CairoSVG and Pillow."""
from copy import deepcopy
from pathlib import Path
import json
import xml.etree.ElementTree as ET
import zipfile

import cairosvg
from PIL import Image, ImageChops, ImageStat

ROOT = Path(__file__).resolve().parent
SVG = '{http://www.w3.org/2000/svg}'
PIVOTS = [(502,398),(536,516),(558,611),(456,652),(455,788),(454,925),(454,962),
          (361,652),(362,787),(360,934),(359,970),(410,641),(298,403),(273,516),
          (242,615),(409,624),(400,374),(408,425),(408,425),(408,425),(400,374)]
PARENTS = ['torso','arm-far-upper','arm-far-lower','pelvis','leg-far-upper','leg-far-lower',
           'ankle-far','pelvis','leg-near-upper','leg-near-lower','ankle-near','torso','torso',
           'arm-near-upper','arm-near-lower',None,'head','torso','head','head','head']


def main():
    root = ET.parse(ROOT / 'milo.svg').getroot()
    groups = root.findall(SVG + 'g')
    assert len(groups) == len(PIVOTS) == len(PARENTS)
    (ROOT / 'png-parts').mkdir(exist_ok=True)
    cairosvg.svg2png(url=str(ROOT / 'milo.svg'), write_to=str(ROOT / 'milo.png'))
    composite = Image.new('RGBA', (800, 1080))
    parts = []
    for index, group in enumerate(groups):
        part_root = deepcopy(root)
        for child in list(part_root):
            if child.tag == SVG + 'g' and child.get('id') != group.get('id'):
                part_root.remove(child)
        filename = f"{index + 1:02d}-{group.get('id')}.png"
        path = ROOT / 'png-parts' / filename
        cairosvg.svg2png(bytestring=ET.tostring(part_root), write_to=str(path))
        part = Image.open(path).convert('RGBA')
        assert part.size == (800, 1080)
        assert part.getchannel('A').getextrema() == (0, 255)
        composite = Image.alpha_composite(composite, part)
        parts.append({'id': group.get('id'), 'file': 'png-parts/' + filename,
                      'suggested_parent': PARENTS[index], 'pivot_canvas_px': PIVOTS[index],
                      'stack_back_to_front': index + 1})
    manifest = {'name': 'Milo', 'canvas': [800, 1080], 'coordinates': 'top-left origin; x right, y down',
                'color_space': 'sRGB', 'license': 'GPL-3.0-or-later',
                'note': 'Manual setup guide. OPEN-TOON does not automatically consume this manifest. Groups are artwork, not bones. Check joint overlaps after posing.',
                'parts': parts}
    (ROOT / 'parts.json').write_text(json.dumps(manifest, indent=2) + '\n')
    white = Image.new('RGBA', composite.size, 'white')
    master = Image.open(ROOT / 'milo.png').convert('RGBA')
    preview = Image.alpha_composite(white, master).convert('RGB')
    preview.save(ROOT / 'preview.png')
    # Compare visible pixels; separate rasterization introduces subpixel rounding.
    diff = ImageChops.difference(preview, Image.alpha_composite(white, composite).convert('RGB'))
    extrema = max(high for _, high in diff.getextrema())
    mean = sum(ImageStat.Stat(diff).mean) / 3
    assert extrema <= 8 and mean < 0.1, (extrema, mean)
    bbox = master.getbbox()
    assert bbox and bbox[0] > 0 and bbox[1] > 0 and bbox[2] < 800 and bbox[3] < 1080
    print(json.dumps({'parts': len(parts), 'canvas': master.size, 'art_bounds': bbox,
                      'composite_max_channel_difference': extrema, 'composite_mean_difference': mean}))
    with zipfile.ZipFile(ROOT / 'milo-import-kit.zip', 'w', zipfile.ZIP_DEFLATED) as archive:
        for path in [ROOT / n for n in ['milo.svg', 'milo.png', 'preview.png', 'parts.json', 'README.md', 'export.py']]:
            archive.write(path, 'milo/' + path.name)
        for path in sorted((ROOT / 'png-parts').glob('*.png')):
            archive.write(path, 'milo/png-parts/' + path.name)


if __name__ == '__main__':
    main()
