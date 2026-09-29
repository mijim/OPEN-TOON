#!/usr/bin/env python3
"""Draw original, continuous toon limbs and joined trouser waist for HM-06."""
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
    def curve(start: tuple[float, float], commands: list[tuple],
              color: tuple[int, ...], border: float = 0) -> None:
        converted = []
        for command in commands:
            points = [p(command[index], command[index + 1])
                      for index in range(1, len(command), 2)]
            converted.append((command[0], *(coordinate for point in points for coordinate in point)))
        art.curve_shape(p(*start), converted, color, border)

    if role.startswith('arm_'):
        # A single tapered sleeve silhouette: no image or outline boundary at the elbow.
        curve((845, 505), [('C', 853, 494, 866, 489, 878, 492),
                           ('C', 898, 494, 916, 502, 925, 513),
                           ('C', 918, 548, 889, 592, 875, 622),
                           ('C', 865, 661, 858, 718, 849, 764),
                           ('L', 812, 760),
                           ('C', 816, 717, 825, 661, 830, 623),
                           ('C', 834, 586, 830, 540, 845, 505)], HOODIE, 4)
        poly([(845, 510), (855, 502), (865, 505), (861, 552),
              (850, 611), (841, 673), (832, 730), (818, 743),
              (828, 672), (835, 601)], HOODIE_LIGHT)
        line([(883, 516), (872, 573), (864, 621), (856, 668)], 2, HOODIE_DARK)
        poly([(813, 744), (851, 749), (849, 763), (812, 759)], HOODIE_DARK, 3)
        line([(817, 753), (846, 755)], 2, ACCENT)
    else:
        # One uninterrupted trouser contour. The fuller knee and narrower
        # ankle keep the folded silhouette legible without a separate cap.
        curve((894, 666), [('C', 909, 655, 936, 657, 946, 680),
                           ('C', 949, 699, 939, 747, 936, 779),
                           ('C', 938, 797, 938, 819, 934, 834),
                           ('C', 932, 868, 929, 909, 930, 935),
                           ('C', 931, 949, 912, 952, 900, 950),
                           ('L', 874, 946),
                           ('C', 870, 931, 875, 896, 876, 864),
                           ('C', 875, 840, 875, 823, 877, 805),
                           ('C', 878, 781, 882, 751, 884, 730),
                           ('C', 884, 703, 882, 682, 894, 666)], PANTS, 4)
        poly([(897, 681), (910, 674), (906, 754), (895, 810),
              (891, 865), (889, 929), (879, 931), (881, 844),
              (885, 797)], PANTS_LIGHT)
        line([(934, 683), (929, 738), (921, 796), (920, 864)], 1.8, INK)
        line([(880, 936), (923, 938)], 1.8, INK)
        line([(885, 942), (918, 943)], 1.2, CREAM)
    return encode(art.pixels)


def draw_pelvis(center: list[int]) -> bytes:
    """Cover the trouser roots with one waist shape that flows into both legs."""
    art = Canvas(SIZE)
    def p(x: float, y: float) -> tuple[float, float]:
        return x - center[0] + SIZE / 2, y - center[1] + SIZE / 2
    def poly(points: list[tuple[float, float]], color: tuple[int, ...]) -> None:
        art.polygon([p(x, y) for x, y in points], color)
    def line(points: list[tuple[float, float]], radius: float,
             color: tuple[int, ...]) -> None:
        art.line([p(x, y) for x, y in points], radius, color)

    # The lower edges deliberately overlap the complete leg drawings. There is
    # no outlined cap across either thigh or a separate codpiece silhouette.
    poly([(901, 635), (1019, 635), (1029, 647), (1034, 677),
          (1038, 707), (1021, 711), (985, 711), (973, 697),
          (960, 690), (947, 697), (935, 711), (899, 711),
          (882, 707), (886, 677), (891, 648)], PANTS)
    poly([(899, 650), (911, 650), (910, 681), (905, 711),
          (892, 711), (895, 681)], PANTS_LIGHT)
    poly([(1021, 650), (1009, 650), (1010, 681), (1015, 711),
          (1028, 711), (1025, 681)], PANTS_LIGHT)
    poly([(945, 651), (975, 651), (979, 677), (960, 690),
          (941, 677)], PANTS)
    line([(900, 641), (889, 651), (884, 679), (881, 707)], 2.2, INK)
    line([(1020, 641), (1031, 651), (1036, 679), (1039, 707)], 2.2, INK)
    line([(946, 690), (960, 682), (974, 690)], 1.6, INK)
    line([(896, 654), (922, 655)], 1.2, PANTS_LIGHT)
    line([(998, 655), (1024, 654)], 1.2, PANTS_LIGHT)
    return encode(art.pixels)


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument('--output', type=Path, default=SPEC.parent / 'parts')
    args = parser.parse_args()
    spec = json.loads(SPEC.read_text(encoding='utf-8'))
    args.output.mkdir(parents=True, exist_ok=True)
    for role, part in spec['limbs'].items():
        (args.output / f'{role}__base.png').write_bytes(draw(role, part['center_px']))
    (args.output / 'pelvis__base.png').write_bytes(draw_pelvis(spec['joined_waist']['center_px']))


if __name__ == '__main__':
    main()
