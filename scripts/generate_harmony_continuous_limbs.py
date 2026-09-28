#!/usr/bin/env python3
"""Draw original, single-sprite toon sleeves and trouser legs for HM-06."""
from __future__ import annotations

import argparse
import json
from pathlib import Path
import struct
import zlib

from generate_harmony_fixture import (Canvas, INK, HOODIE, HOODIE_LIGHT,
                                      HOODIE_DARK, PANTS, PANTS_LIGHT,
                                      ACCENT, CREAM, png_chunk)

ROOT = Path(__file__).resolve().parents[1]
SPEC = ROOT / 'tests/fixtures/harmony-continuous-limbs/rig.json'
SIZE = 512


def encode(pixels: bytes) -> bytes:
    rows = b''.join(b'\0' + pixels[y * SIZE * 4:(y + 1) * SIZE * 4]
                    for y in range(SIZE))
    header = struct.pack('>2I5B', SIZE, SIZE, 8, 6, 0, 0, 0)
    return (b'\x89PNG\r\n\x1a\n' + png_chunk(b'IHDR', header) +
            png_chunk(b'sRGB', b'\0') +
            png_chunk(b'IDAT', zlib.compress(rows, 9)) +
            png_chunk(b'IEND', b''))


def draw(role: str, center: list[int]) -> bytes:
    art = Canvas(SIZE)
    right = role.endswith('right')
    def p(x: float, y: float) -> tuple[float, float]:
        scene_x = 1920 - x if right else x
        return scene_x - center[0] + SIZE / 2, y - center[1] + SIZE / 2
    def poly(points: list[tuple[float, float]], color: tuple[int, ...], border: float = 0) -> None:
        converted = [p(x, y) for x, y in points]
        if border:
            art.outlined_polygon(converted, color, border)
        else:
            art.polygon(converted, color)
    def line(points: list[tuple[float, float]], radius: float, color: tuple[int, ...]) -> None:
        art.line([p(x, y) for x, y in points], radius, color)

    if role.startswith('arm_'):
        # A single tapered sleeve silhouette: no image or outline boundary at the elbow.
        poly([(845, 505), (862, 493), (892, 497), (925, 513),
              (917, 541), (889, 594), (875, 622), (861, 690),
              (852, 749), (849, 764), (812, 760), (813, 742),
              (823, 686), (830, 623), (833, 586), (836, 535)], HOODIE, 4)
        poly([(845, 510), (855, 502), (865, 505), (861, 552),
              (850, 611), (841, 673), (832, 730), (818, 743),
              (828, 672), (835, 601)], HOODIE_LIGHT)
        line([(883, 516), (872, 573), (864, 621), (856, 668)], 2, HOODIE_DARK)
        poly([(813, 744), (851, 749), (849, 763), (812, 759)], HOODIE_DARK, 3)
        line([(817, 753), (846, 755)], 2, ACCENT)
    else:
        # One uninterrupted trouser leg from hip through knee to ankle.
        poly([(894, 666), (920, 657), (943, 668), (947, 692),
              (939, 743), (934, 800), (931, 850), (930, 935),
              (925, 949), (874, 948), (873, 932), (875, 852),
              (878, 798), (883, 742), (886, 688)], PANTS, 4)
        poly([(897, 681), (910, 674), (906, 754), (895, 810),
              (891, 865), (889, 929), (879, 931), (881, 844),
              (885, 797)], PANTS_LIGHT)
        line([(934, 683), (929, 738), (921, 796), (920, 864)], 1.8, INK)
        line([(880, 936), (923, 938)], 1.8, INK)
        line([(885, 942), (918, 943)], 1.2, CREAM)
    return encode(art.pixels)


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument('--output', type=Path, default=SPEC.parent / 'parts')
    args = parser.parse_args()
    spec = json.loads(SPEC.read_text(encoding='utf-8'))
    args.output.mkdir(parents=True, exist_ok=True)
    for role, part in spec['limbs'].items():
        (args.output / f'{role}__base.png').write_bytes(draw(role, part['center_px']))


if __name__ == '__main__':
    main()
