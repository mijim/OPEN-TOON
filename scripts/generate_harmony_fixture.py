#!/usr/bin/env python3
"""Generate original, redistributable HM-00 PNG and PCM reference assets."""
from __future__ import annotations

import argparse
import hashlib
import json
import math
from pathlib import Path
import struct
import wave
import zlib

ROOT = Path(__file__).resolve().parents[1]
SPEC = ROOT / 'tests/fixtures/harmony-moment/shot.json'
SIZE = 256


def png_chunk(kind: bytes, body: bytes) -> bytes:
    return struct.pack('>I', len(body)) + kind + body + struct.pack('>I', zlib.crc32(kind + body))


def rgba_png(pixels: bytes) -> bytes:
    rows = b''.join(b'\0' + pixels[y * SIZE * 4:(y + 1) * SIZE * 4] for y in range(SIZE))
    header = struct.pack('>2I5B', SIZE, SIZE, 8, 6, 0, 0, 0)
    return (b'\x89PNG\r\n\x1a\n' + png_chunk(b'IHDR', header) +
            png_chunk(b'sRGB', b'\0') + png_chunk(b'IDAT', zlib.compress(rows, 9)) + png_chunk(b'IEND', b''))


INK = (34, 33, 43, 255)
HOODIE = (43, 104, 110, 255)
HOODIE_LIGHT = (77, 151, 151, 255)
HOODIE_DARK = (28, 76, 86, 255)
SKIN = (228, 173, 129, 255)
SKIN_LIGHT = (247, 203, 160, 255)
SKIN_SHADE = (192, 117, 89, 255)
CHEEK = (222, 137, 112, 255)
HAIR = (69, 48, 58, 255)
HAIR_LIGHT = (124, 76, 75, 255)
CREAM = (249, 228, 194, 255)
ACCENT = (228, 103, 72, 255)
ACCENT_DARK = (176, 67, 63, 255)
PANTS = (43, 50, 62, 255)
PANTS_LIGHT = (77, 88, 100, 255)
MOUTH = (89, 48, 59, 255)
TONGUE = (204, 98, 105, 255)


class Canvas:
    """Opaque layered vector shapes on the registered transparent PNG canvas."""

    def __init__(self) -> None:
        self.pixels = bytearray(SIZE * SIZE * 4)

    def fill(self, bounds: tuple[float, float, float, float], hit, color: tuple[int, ...]) -> None:
        left = max(0, math.floor(bounds[0]))
        top = max(0, math.floor(bounds[1]))
        right = min(SIZE, math.ceil(bounds[2]))
        bottom = min(SIZE, math.ceil(bounds[3]))
        ink = bytes(color)
        for y in range(top, bottom):
            for x in range(left, right):
                if hit(x + 0.5, y + 0.5):
                    offset = (y * SIZE + x) * 4
                    self.pixels[offset:offset + 4] = ink

    def ellipse(self, cx: float, cy: float, rx: float, ry: float, color: tuple[int, ...]) -> None:
        self.fill((cx - rx, cy - ry, cx + rx, cy + ry),
                  lambda x, y: ((x - cx) / rx) ** 2 + ((y - cy) / ry) ** 2 <= 1, color)

    def capsule(self, ax: float, ay: float, bx: float, by: float,
                radius: float, color: tuple[int, ...]) -> None:
        dx, dy = bx - ax, by - ay
        squared = dx * dx + dy * dy
        if squared == 0:
            self.ellipse(ax, ay, radius, radius, color)
            return

        def hit(x: float, y: float) -> bool:
            fraction = ((x - ax) * dx + (y - ay) * dy) / squared
            fraction = max(0.0, min(1.0, fraction))
            distance_x = x - ax - fraction * dx
            distance_y = y - ay - fraction * dy
            return distance_x * distance_x + distance_y * distance_y <= radius * radius

        self.fill((min(ax, bx) - radius, min(ay, by) - radius,
                   max(ax, bx) + radius, max(ay, by) + radius),
                  hit, color)

    def polygon(self, points: list[tuple[float, float]], color: tuple[int, ...]) -> None:
        edges = list(zip(points, points[1:] + points[:1]))

        def hit(x: float, y: float) -> bool:
            inside = False
            for (ax, ay), (bx, by) in edges:
                if (ay > y) != (by > y) and x < ax + (y - ay) * (bx - ax) / (by - ay):
                    inside = not inside
            return inside

        self.fill((min(p[0] for p in points), min(p[1] for p in points),
                   max(p[0] for p in points), max(p[1] for p in points)), hit, color)

    def line(self, points: list[tuple[float, float]], radius: float,
             color: tuple[int, ...]) -> None:
        for (ax, ay), (bx, by) in zip(points, points[1:]):
            self.capsule(ax, ay, bx, by, radius, color)

    def outlined_ellipse(self, cx: float, cy: float, rx: float, ry: float,
                         color: tuple[int, ...], border: float = 3) -> None:
        self.ellipse(cx, cy, rx, ry, INK)
        self.ellipse(cx, cy, rx - border, ry - border, color)

    def outlined_capsule(self, ax: float, ay: float, bx: float, by: float,
                         radius: float, color: tuple[int, ...], border: float = 3) -> None:
        self.capsule(ax, ay, bx, by, radius, INK)
        self.capsule(ax, ay, bx, by, radius - border, color)

    def outlined_polygon(self, points: list[tuple[float, float]],
                         color: tuple[int, ...], border: float = 3) -> None:
        self.polygon(points, color)
        self.line(points + [points[0]], border / 2, INK)

    def curve_shape(self, start: tuple[float, float], commands: list[tuple],
                    color: tuple[int, ...], border: float = 0) -> None:
        points = [start]
        for command in commands:
            if command[0] == 'L':
                points.append((command[1], command[2]))
                continue
            anchor = points[-1]
            control_a = (command[1], command[2])
            control_b = (command[3], command[4])
            end = (command[5], command[6])
            for step in range(1, 9):
                t = step / 8
                u = 1 - t
                points.append((u ** 3 * anchor[0] + 3 * u * u * t * control_a[0] +
                               3 * u * t * t * control_b[0] + t ** 3 * end[0],
                               u ** 3 * anchor[1] + 3 * u * u * t * control_a[1] +
                               3 * u * t * t * control_b[1] + t ** 3 * end[1]))
        self.polygon(points, color)
        if border:
            self.line(points + [points[0]], border / 2, INK)

    def side_curve_shape(self, start: tuple[float, float], commands: list[tuple],
                         color: tuple[int, ...], border: float, right: bool) -> None:
        if right:
            start = (SIZE - start[0], start[1])
            commands = [tuple(SIZE - value if index % 2 else value
                              for index, value in enumerate(command))
                        for command in commands]
        self.curve_shape(start, commands, color, border)


def artwork(role: str, variant: str) -> bytes:
    art = Canvas()
    if role == 'torso':
        art.curve_shape((83, 35), [('C', 65, 39, 61, 61, 55, 87),
                                  ('L', 67, 112), ('L', 78, 205),
                                  ('C', 100, 219, 155, 222, 181, 205),
                                  ('L', 190, 110), ('L', 202, 88),
                                  ('C', 198, 62, 186, 40, 169, 35),
                                  ('C', 145, 24, 111, 31, 83, 35)], HOODIE, 4)
        art.curve_shape((81, 41), [('C', 68, 57, 62, 81, 68, 102),
                                  ('L', 87, 116), ('L', 98, 66),
                                  ('L', 117, 44), ('L', 81, 41)], HOODIE_LIGHT)
        art.curve_shape((168, 39), [('C', 190, 55, 196, 79, 187, 103),
                                   ('L', 172, 110), ('L', 156, 61),
                                   ('L', 139, 42), ('L', 168, 39)], HOODIE_DARK)
        art.curve_shape((99, 43), [('C', 110, 67, 118, 80, 128, 80),
                                  ('C', 138, 78, 151, 61, 158, 42),
                                  ('L', 167, 51), ('C', 159, 82, 143, 102, 127, 105),
                                  ('C', 109, 101, 93, 83, 89, 53), ('L', 99, 43)],
                        HOODIE_DARK, 2)
        art.curve_shape((109, 72), [('C', 119, 89, 135, 93, 148, 72),
                                   ('L', 155, 82), ('C', 142, 110, 116, 110, 102, 83),
                                   ('L', 109, 72)], HOODIE_LIGHT)
        art.line([(113, 99), (110, 135)], 1.5, ACCENT)
        art.line([(145, 99), (148, 133)], 1.5, ACCENT)
        art.ellipse(110, 137, 2.5, 3, ACCENT_DARK)
        art.ellipse(148, 135, 2.5, 3, ACCENT_DARK)
        art.curve_shape((93, 161), [('C', 108, 154, 126, 168, 142, 165),
                                   ('L', 165, 159), ('L', 159, 192),
                                   ('C', 133, 201, 111, 195, 95, 188),
                                   ('L', 93, 161)], HOODIE_LIGHT, 2)
        art.line([(98, 163), (117, 174), (141, 174), (163, 160)], 1.6, HOODIE_DARK)
        art.polygon([(153, 177), (165, 172), (163, 193), (148, 199)], HOODIE_DARK)
        art.line([(82, 207), (113, 215), (154, 215), (181, 206)], 2.5, INK)
    elif role == 'pelvis':
        art.curve_shape((80, 77), [('L', 178, 77), ('L', 184, 121),
                                  ('C', 174, 161, 157, 177, 135, 180),
                                  ('L', 105, 177), ('C', 83, 160, 72, 128, 80, 77)],
                        PANTS, 3)
        art.polygon([(82, 77), (177, 77), (178, 93), (79, 94)], HOODIE_DARK)
        art.line([(89, 96), (95, 149), (113, 171)], 2, PANTS_LIGHT)
        art.line([(168, 99), (161, 153), (147, 172)], 2, PANTS_LIGHT)
        art.line([(128, 100), (126, 145)], 1.5, INK)
    elif role == 'neck':
        art.curve_shape((105, 92), [('L', 152, 92), ('L', 147, 171),
                                   ('C', 133, 181, 120, 181, 107, 169),
                                   ('L', 105, 92)], SKIN, 3)
        art.polygon([(111, 129), (142, 147), (147, 164), (114, 164)], SKIN_SHADE)
        art.polygon([(99, 160), (126, 183), (117, 188), (89, 169)], CREAM)
        art.polygon([(155, 160), (132, 183), (142, 188), (166, 168)], CREAM)
    elif role == 'head':
        side = variant == 'three_quarter'
        shift = 11 if side else 0
        art.outlined_ellipse(62 + shift, 137, 12, 21, SKIN, 3)
        art.line([(57 + shift, 129), (65 + shift, 137), (59 + shift, 146)], 1.7, SKIN_SHADE)
        if side:
            outline = [(110, 51), (151, 42), (181, 55), (199, 83),
                       (205, 120), (220, 140), (206, 151), (193, 180),
                       (160, 207), (126, 202), (96, 182), (78, 149),
                       (76, 109), (88, 75)]
        else:
            outline = [(114, 47), (151, 44), (181, 59), (197, 88),
                       (200, 128), (192, 166), (171, 193), (139, 208),
                       (105, 199), (78, 182), (59, 153), (56, 114),
                       (69, 77), (91, 57)]
        art.outlined_polygon(outline, SKIN, 3.5)
        art.curve_shape((85 + shift, 87), [('C', 85 + shift, 69, 124 + shift, 61,
                                           148 + shift, 68),
                                          ('C', 117 + shift, 78, 107 + shift, 106,
                                           94 + shift, 126), ('L', 85 + shift, 87)],
                        SKIN_LIGHT)
        art.ellipse(169 + shift, 151, 9, 7, CHEEK)
        art.line([(134 + shift, 122), (147 + shift, 143),
                  (155 + shift, 150), (144 + shift, 153)], 1.7, INK)
        art.line([(106 + shift, 184), (128 + shift, 193),
                  (149 + shift, 189)], 1.5, SKIN_SHADE)
        art.outlined_ellipse(193 + shift, 134, 9, 17, SKIN, 2)
        art.line([(194 + shift, 130), (189 + shift, 138)], 1.3, SKIN_SHADE)
    elif role == 'hair':
        shift = 11 if variant == 'three_quarter' else 0
        art.curve_shape((56 + shift, 121),
                        [('C', 43 + shift, 94, 54 + shift, 62, 84 + shift, 49),
                         ('C', 111 + shift, 13, 158 + shift, 26, 178 + shift, 41),
                         ('C', 205 + shift, 40, 213 + shift, 69, 208 + shift, 100),
                         ('L', 193 + shift, 89), ('L', 188 + shift, 116),
                         ('C', 166 + shift, 95, 163 + shift, 83, 137 + shift, 82),
                         ('C', 125 + shift, 102, 107 + shift, 110, 83 + shift, 100),
                         ('C', 79 + shift, 120, 70 + shift, 139, 59 + shift, 147),
                         ('L', 56 + shift, 121)], HAIR, 4)
        art.curve_shape((76 + shift, 75),
                        [('C', 102 + shift, 39, 136 + shift, 40, 156 + shift, 46),
                         ('C', 132 + shift, 48, 109 + shift, 61, 92 + shift, 89),
                         ('L', 76 + shift, 75)], HAIR_LIGHT)
        art.curve_shape((82 + shift, 102),
                        [('C', 91 + shift, 109, 91 + shift, 124, 79 + shift, 133),
                         ('L', 62 + shift, 137), ('C', 64 + shift, 118, 72 + shift, 107,
                          82 + shift, 102)], HAIR)
        art.line([(101 + shift, 57), (135 + shift, 39),
                  (165 + shift, 44)], 1.7, HAIR_LIGHT)
    elif role == 'eyes':
        side = variant == 'three_quarter'
        eyes = [(112, 126, 9, 8), (163, 125, 9, 9)] if not side else [
            (124, 126, 7, 8), (175, 125, 9, 9)]
        for x, y, rx, ry in eyes:
            art.outlined_ellipse(x, y, rx, ry, CREAM, 2)
            art.ellipse(x + (2 if side else 0), y + 1, 3.5, 4.5, INK)
            art.ellipse(x - 1 + (2 if side else 0), y - 2, 1.5, 1.5, CREAM)
            art.line([(x - rx - 3, y - ry + 1), (x + rx + 2, y - ry + 1)], 1.7, INK)
        art.line([(95 if not side else 108, 107), (117 if not side else 128, 103)], 3, HAIR)
        art.line([(149 if not side else 161, 103), (177 if not side else 188, 106)], 3, HAIR)
    elif role == 'mouth':
        view, choice = variant.split('__')
        cx = 149 if view == 'three_quarter' else 128
        cy = 174
        if choice == 'rest':
            art.line([(cx - 18, cy - 4), (cx - 10, cy + 1), (cx, cy + 4),
                      (cx + 10, cy + 1), (cx + 18, cy - 4)], 2.4, MOUTH)
            art.ellipse(cx - 19, cy - 3, 3, 2, SKIN_SHADE)
            art.ellipse(cx + 20, cy - 3, 3, 2, SKIN_SHADE)
        elif choice == 'mbp':
            art.capsule(cx - 17, cy, cx + 17, cy, 3.4, MOUTH)
            art.line([(cx - 12, cy + 6), (cx + 12, cy + 6)], 1.3, SKIN_SHADE)
        elif choice == 'fv':
            art.outlined_ellipse(cx, cy, 22, 8, MOUTH, 2)
            art.polygon([(cx - 17, cy - 4), (cx + 16, cy - 4),
                         (cx + 12, cy), (cx - 12, cy)], CREAM)
        else:
            rx, ry = {'ee': (31, 9), 'ah': (24, 24), 'oh': (16, 20),
                      'l': (20, 13), 'wide': (34, 17)}[choice]
            art.outlined_ellipse(cx, cy, rx, ry, MOUTH, 3)
            if choice in {'ee', 'ah', 'l', 'wide'}:
                art.ellipse(cx, cy + ry * .58, rx * .55, max(2, ry * .30), TONGUE)
                art.polygon([(cx - rx * .71, cy - ry * .72),
                             (cx + rx * .71, cy - ry * .72),
                             (cx + rx * .56, cy - ry * .18),
                             (cx - rx * .56, cy - ry * .18)], CREAM)
    elif role.startswith('hand_'):
        right = role.endswith('right')
        mirror = lambda x: SIZE - x if right else x
        if variant == 'open':
            points = [(112, 107), (144, 107), (148, 137), (151, 166),
                      (149, 188), (143, 191), (138, 188), (136, 170),
                      (135, 194), (128, 198), (122, 194), (121, 171),
                      (119, 194), (112, 195), (106, 189), (105, 171),
                      (102, 185), (95, 182), (94, 173), (101, 148),
                      (80, 157), (71, 153), (72, 146), (99, 130)]
            art.outlined_polygon([(mirror(x), y) for x, y in points], SKIN, 2)
            for x in (108, 120, 134):
                art.line([(mirror(x), 159), (mirror(x + 1), 181)], 1, SKIN_SHADE)
            art.line([(mirror(105), 143), (mirror(118), 152)], 1.4, SKIN_SHADE)
        elif variant == 'fist':
            points = [(111, 107), (145, 107), (153, 145), (152, 165),
                      (145, 181), (117, 184), (103, 173), (100, 152),
                      (108, 139)]
            art.outlined_polygon([(mirror(x), y) for x, y in points], SKIN, 2.5)
            art.line([(mirror(108), 151), (mirror(137), 150)], 1.5, INK)
            for x in (112, 122, 132, 142):
                art.line([(mirror(x), 151), (mirror(x + 2), 166)], 1.3, SKIN_SHADE)
        else:
            points = [(111, 107), (145, 107), (149, 138), (164, 131),
                      (201, 122), (207, 128), (205, 135), (170, 150),
                      (150, 162), (145, 180), (119, 182), (104, 171),
                      (103, 149)]
            art.outlined_polygon([(mirror(x), y) for x, y in points], SKIN, 2)
            for x in (114, 126, 138):
                art.line([(mirror(x), 152), (mirror(x + 2), 172)], 1.2, SKIN_SHADE)
        cuff = [(105, 98), (151, 98), (151, 116), (105, 116)]
        art.outlined_polygon([(mirror(x), y) for x, y in cuff], HOODIE_DARK, 2)
        art.line([(mirror(109), 106), (mirror(148), 106)], 1.7, ACCENT)
    elif role.startswith('upper_arm_'):
        left = role.endswith('left')
        art.side_curve_shape((121, 44),
                             [('C', 106, 41, 96, 58, 95, 79),
                              ('L', 86, 145), ('C', 84, 165, 93, 183, 111, 186),
                              ('C', 131, 190, 143, 169, 145, 150),
                              ('L', 167, 82), ('C', 176, 60, 158, 45, 144, 43),
                             ('L', 121, 44)], HOODIE, 0, not left)
        edge = [(96, 77), (86, 145), (93, 172)]
        other = [(166, 82), (145, 150), (139, 171)]
        if not left:
            edge = [(SIZE - x, y) for x, y in edge]
            other = [(SIZE - x, y) for x, y in other]
        art.line(edge, 2, INK)
        art.line(other, 2, INK)
        art.side_curve_shape((105, 66),
                             [('C', 101, 97, 94, 135, 96, 155),
                              ('C', 98, 162, 103, 166, 109, 166),
                              ('L', 126, 65), ('L', 105, 66)],
                             HOODIE_LIGHT, 0, not left)
        art.line(([(99, 149), (119, 160), (138, 158)] if left else
                  [(157, 149), (137, 160), (118, 158)]), 1.7, HOODIE_DARK)
    elif role.startswith('lower_arm_'):
        left = role.endswith('left')
        art.side_curve_shape((118, 39),
                             [('C', 103, 41, 97, 58, 98, 78),
                              ('L', 94, 165), ('C', 95, 184, 104, 199, 119, 207),
                              ('L', 140, 207), ('C', 151, 193, 153, 177, 150, 158),
                              ('L', 159, 82), ('C', 161, 55, 146, 38, 118, 39)],
                             HOODIE, 0, not left)
        edge = [(99, 57), (96, 111), (94, 165), (101, 190)]
        other = [(155, 58), (153, 113), (151, 167), (145, 191)]
        if not left:
            edge = [(SIZE - x, y) for x, y in edge]
            other = [(SIZE - x, y) for x, y in other]
        art.line(edge, 2, INK)
        art.line(other, 2, INK)
        art.side_curve_shape((107, 59),
                             [('C', 106, 96, 101, 136, 104, 170),
                              ('L', 115, 181), ('L', 125, 64), ('L', 107, 59)],
                             HOODIE_LIGHT, 0, not left)
        cuff = [(96, 177), (151, 177), (149, 197), (102, 197)]
        if not left:
            cuff = [(SIZE - x, y) for x, y in cuff]
        art.outlined_polygon(cuff, HOODIE_DARK, 2)
        art.line(([(99, 185), (148, 185)] if left else
                  [(157, 185), (108, 185)]), 2.2, ACCENT)
    elif role.startswith('upper_leg_'):
        left = role.endswith('left')
        art.side_curve_shape((120, 31),
                             [('C', 103, 30, 91, 48, 94, 67),
                              ('L', 86, 169), ('C', 84, 187, 91, 204, 104, 213),
                              ('L', 129, 215), ('C', 137, 194, 144, 178, 145, 157),
                              ('L', 161, 62), ('C', 165, 41, 148, 30, 120, 31)],
                             PANTS, 0, not left)
        edge = [(94, 63), (86, 169), (91, 196)]
        other = [(160, 64), (145, 157), (136, 196)]
        if not left:
            edge = [(SIZE - x, y) for x, y in edge]
            other = [(SIZE - x, y) for x, y in other]
        art.line(edge, 2, INK)
        art.line(other, 2, INK)
        art.line(([(105, 69), (98, 173), (106, 199)] if left else
                  [(151, 69), (158, 173), (150, 199)]), 2.5, PANTS_LIGHT)
        art.line(([(95, 183), (119, 186), (131, 181)] if left else
                  [(161, 183), (137, 186), (125, 181)]), 1.7, INK)
    elif role.startswith('lower_leg_'):
        left = role.endswith('left')
        art.side_curve_shape((110, 38),
                             [('C', 99, 39, 97, 60, 99, 80),
                              ('L', 98, 181), ('C', 96, 203, 107, 213, 120, 215),
                              ('L', 145, 214), ('C', 151, 200, 154, 182, 152, 160),
                              ('L', 152, 74), ('C', 151, 53, 140, 38, 110, 38)],
                             PANTS, 0, not left)
        edge = [(99, 72), (98, 181), (102, 203)]
        other = [(152, 74), (152, 160), (150, 201)]
        if not left:
            edge = [(SIZE - x, y) for x, y in edge]
            other = [(SIZE - x, y) for x, y in other]
        art.line(edge, 2, INK)
        art.line(other, 2, INK)
        art.line(([(111, 75), (109, 167), (113, 192)] if left else
                  [(145, 75), (147, 167), (143, 192)]), 2.5, PANTS_LIGHT)
        art.line(([(101, 192), (139, 196)] if left else
                  [(155, 192), (117, 196)]), 1.5, INK)
    elif role.startswith('foot_'):
        left = role.endswith('left')
        points = [(108, 116), (153, 116), (159, 142), (173, 150), (183, 166),
                  (177, 179), (68, 179), (60, 171), (69, 151), (98, 141)]
        if not left:
            points = [(256 - x, y) for x, y in points]
        art.outlined_polygon(points, INK)
        art.polygon([(min(x for x, _ in points) + 4, 164),
                     (max(x for x, _ in points) - 4, 164),
                     (max(x for x, _ in points) - 8, 176),
                     (min(x for x, _ in points) + 8, 176)], CREAM)
        for y in (140, 148, 156):
            art.line([(113 if left else 143, y), (139 if left else 117, y + 2)], 1.4, CREAM)
    else:
        raise ValueError(f'Unknown part role: {role}')
    return rgba_png(art.pixels)


def audio(path: Path, sample_count: int, sample_rate: int, markers: set[int], pitch: int = 660) -> None:
    pulse_samples = sample_rate // 12
    pulse = [int(11000 * (1 - i / pulse_samples) * math.sin(2 * math.pi * pitch * i / sample_rate))
             for i in range(pulse_samples)]
    with wave.open(str(path), 'wb') as out:
        out.setnchannels(1)
        out.setsampwidth(2)
        out.setframerate(sample_rate)
        for start in range(0, sample_count, sample_rate):
            count = min(sample_rate, sample_count - start)
            block = bytearray(count * 2)
            for marker in markers:
                begin = max(start, marker)
                end = min(start + count, marker + pulse_samples)
                for index in range(begin, end):
                    struct.pack_into('<h', block, (index - start) * 2, pulse[index - marker])
            out.writeframesraw(block)
        out.writeframes(b'')


def file_hash(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open('rb') as source:
        for chunk in iter(lambda: source.read(1024 * 1024), b''):
            digest.update(chunk)
    return digest.hexdigest()


def preview(spec: dict, parts: Path, pose: dict) -> bytes:
    width, height = spec['canvas']['width'], spec['canvas']['height']
    pixels = bytearray(b'\xff\xff\xff\xff' * (width * height))
    zoom = pose['camera_zoom']
    for role in spec['reference_paint_order']:
        variant = ('__' + pose['view'] + '__' + pose['mouth'] if role == 'mouth' else
                   '__' + pose['view'] if role in {'head', 'hair', 'eyes'} else
                   '__' + pose['right_hand'] if role == 'hand_right' else
                   '__open' if role == 'hand_left' else '__base')
        data = (parts / f'{role}{variant}.png').read_bytes()
        # The generator emits one sRGB chunk before the single IDAT chunk.
        length = struct.unpack_from('>I', data, 46)[0]
        raw = zlib.decompress(data[54:54 + length])
        center_x, center_y = spec['reference_centers_px'][role]
        center_x += pose['root_dx_px']
        for sy in range(SIZE):
            row = sy * (SIZE * 4 + 1) + 1
            dy = round((center_y + sy - SIZE // 2 - height / 2) * zoom + height / 2)
            if not 0 <= dy < height:
                continue
            for sx in range(SIZE):
                source = row + sx * 4
                if raw[source + 3] == 0:
                    continue
                dx = round((center_x + sx - SIZE // 2 - width / 2) * zoom + width / 2)
                if 0 <= dx < width:
                    target = (dy * width + dx) * 4
                    pixels[target:target + 4] = raw[source:source + 4]
    rows = b''.join(b'\0' + pixels[y * width * 4:(y + 1) * width * 4] for y in range(height))
    header = struct.pack('>2I5B', width, height, 8, 6, 0, 0, 0)
    return (b'\x89PNG\r\n\x1a\n' + png_chunk(b'IHDR', header) +
            png_chunk(b'sRGB', b'\0') + png_chunk(b'IDAT', zlib.compress(rows, 6)) + png_chunk(b'IEND', b''))


def generate(destination: Path, long_audio: bool, previews: bool = False) -> dict:
    spec = json.loads(SPEC.read_text(encoding='utf-8'))
    destination.mkdir(parents=True, exist_ok=True)
    parts = destination / 'parts'
    parts.mkdir(exist_ok=True)
    files = {}

    def save(path: Path, data: bytes) -> None:
        path.write_bytes(data)
        files[str(path.relative_to(destination))] = hashlib.sha256(data).hexdigest()

    for role in spec['part_roles']:
        if role == 'mouth':
            variants = [f'{view}__{shape}' for view in spec['substitutions']['coordinated_views']
                        for shape in spec['substitutions']['mouth']]
        else:
            variants = spec['substitutions'].get(role, ['front'] if role in {'head', 'hair', 'eyes'} else ['base'])
        for variant in variants:
            path = parts / f'{role}__{variant}.png'
            save(path, artwork(role, variant))
    for role in ('head', 'hair', 'eyes'):
        path = parts / f'{role}__three_quarter.png'
        save(path, artwork(role, 'three_quarter'))
    if previews:
        for pose in spec['reference_key_poses']:
            save(destination / f"reference_{pose['frame']:04d}.png", preview(spec, parts, pose))

    dialogue = destination / 'dialogue_reference_24.wav'
    fractional_dialogue = destination / 'dialogue_reference_24000_1001.wav'
    music = destination / 'music_reference.wav'
    audio(dialogue, spec['audio']['asset_samples'], 48000,
          {frame * 2000 for frame in spec['audio']['cue_frames']})
    audio(fractional_dialogue, spec['audio']['asset_samples'], 48000,
          {frame * 2002 for frame in spec['audio']['cue_frames']})
    audio(music, spec['audio']['asset_samples'], 48000,
          {second * 48000 for second in (0, 4, 8, 12, 16)}, 330)
    for path in (dialogue, fractional_dialogue, music):
        files[path.name] = file_hash(path)
    if long_audio:
        drift = destination / 'drift_10_minutes.wav'
        audio(drift, spec['audio']['drift_seconds'] * 48000, 48000,
              {second * 48000 for second in range(0, spec['audio']['drift_seconds'],
                                                 spec['audio']['drift_marker_seconds'])}, 880)
        files[drift.name] = file_hash(drift)

    provenance = {'source': str(SPEC.relative_to(ROOT)), 'license': 'GPL-3.0-or-later',
                  'generated_by': 'scripts/generate_harmony_fixture.py', 'files_sha256': files}
    (destination / 'PROVENANCE.json').write_text(json.dumps(provenance, indent=2) + '\n', encoding='utf-8')
    return provenance


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, default=ROOT / 'build/harmony-moment-fixture')
    parser.add_argument('--long-audio', action='store_true', help='also write a 600-second drift WAV')
    parser.add_argument('--previews', action='store_true', help='render five 1080p reference poses')
    args = parser.parse_args()
    result = generate(args.output, args.long_audio, args.previews)
    print(f"Generated {len(result['files_sha256'])} verified asset hashes in {args.output}")
