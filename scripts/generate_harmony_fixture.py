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


INK = (29, 35, 48, 255)
COAT = (53, 91, 119, 255)
COAT_LIGHT = (83, 129, 151, 255)
COAT_DARK = (34, 67, 95, 255)
SKIN = (226, 174, 136, 255)
SKIN_LIGHT = (247, 205, 166, 255)
SKIN_SHADE = (194, 128, 100, 255)
CHEEK = (220, 151, 125, 255)
HAIR = (57, 48, 55, 255)
HAIR_LIGHT = (110, 84, 76, 255)
SHIRT = (247, 240, 222, 255)
TEAL = (52, 166, 155, 255)
TEAL_DARK = (36, 125, 126, 255)
PANTS = (42, 52, 73, 255)
PANTS_LIGHT = (67, 80, 103, 255)
MOUTH = (91, 47, 61, 255)
TONGUE = (199, 103, 112, 255)


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


def artwork(role: str, variant: str) -> bytes:
    art = Canvas()
    if role == 'torso':
        art.outlined_polygon([(91, 37), (165, 37), (190, 56), (202, 96),
                              (180, 123), (174, 217), (82, 217), (76, 123),
                              (54, 96), (66, 56)], COAT)
        art.polygon([(99, 40), (157, 40), (153, 200), (101, 200)], SHIRT)
        art.polygon([(66, 60), (94, 45), (127, 86), (104, 119), (76, 94)], COAT_LIGHT)
        art.polygon([(190, 60), (162, 45), (129, 86), (152, 119), (181, 94)], COAT_DARK)
        art.line([(128, 89), (128, 206)], 2, INK)
        art.polygon([(128, 86), (143, 104), (132, 133), (126, 133), (113, 104)], TEAL_DARK)
        art.polygon([(128, 90), (138, 104), (129, 126), (125, 126), (118, 104)], TEAL)
        for y in (146, 180):
            art.ellipse(137, y, 4, 4, INK)
            art.ellipse(136, y - 1, 1.5, 1.5, SHIRT)
        art.line([(71, 169), (93, 171), (99, 181)], 2, COAT_DARK)
        art.line([(185, 169), (163, 171), (157, 181)], 2, COAT_LIGHT)
        art.line([(90, 210), (166, 210)], 2, INK)
    elif role == 'pelvis':
        art.outlined_polygon([(80, 73), (176, 73), (186, 119), (173, 163),
                              (147, 179), (108, 179), (82, 163), (70, 119)], PANTS)
        art.polygon([(78, 81), (178, 81), (181, 99), (75, 99)], COAT_DARK)
        art.outlined_polygon([(118, 85), (138, 85), (138, 103), (118, 103)], TEAL, 2)
        art.line([(127, 106), (127, 168)], 1.5, PANTS_LIGHT)
    elif role == 'neck':
        art.outlined_capsule(128, 91, 128, 160, 22, SKIN)
        art.polygon([(108, 130), (128, 149), (148, 130), (142, 158), (114, 158)], SKIN_SHADE)
        art.polygon([(100, 158), (127, 183), (113, 185), (92, 167)], SHIRT)
        art.polygon([(156, 158), (129, 183), (143, 185), (164, 167)], SHIRT)
    elif role == 'head':
        side = variant == 'three_quarter'
        shift = 12 if side else 0
        for ear in (64 + shift, 192 + shift):
            art.outlined_ellipse(ear, 137, 15 if ear < 128 else 12, 22, SKIN_SHADE)
            art.ellipse(ear, 137, 7, 13, SKIN)
        if side:
            outline = [(135, 47), (171, 52), (196, 73), (209, 103), (207, 130),
                       (223, 139), (211, 151), (204, 177), (180, 198),
                       (149, 207), (112, 197), (84, 174), (72, 141),
                       (73, 104), (91, 72), (111, 53)]
        else:
            outline = [(128, 46), (164, 51), (190, 72), (202, 105), (200, 144),
                       (186, 177), (162, 201), (129, 210), (94, 202),
                       (70, 177), (56, 144), (55, 105), (67, 73), (92, 52)]
        art.outlined_polygon(outline, SKIN, 4)
        art.ellipse(104 + shift, 119, 21, 31, SKIN_LIGHT)
        art.ellipse(175 + shift, 154, 12, 17, CHEEK)
        art.ellipse(92 + shift, 154, 11, 6, CHEEK)
        art.line([(143 + shift, 130), (149 + shift, 149), (144 + shift, 154)], 2, SKIN_SHADE)
        art.ellipse(143 + shift, 156, 5, 3, SKIN_LIGHT)
        art.line([(98 + shift, 188), (122 + shift, 196), (146 + shift, 194)], 1.5, SKIN_SHADE)
    elif role == 'hair':
        shift = 12 if variant == 'three_quarter' else 0
        outline = [(53 + shift, 119), (52 + shift, 84), (68 + shift, 55),
                   (97 + shift, 35), (139 + shift, 29), (182 + shift, 43),
                   (204 + shift, 66), (209 + shift, 102), (194 + shift, 93),
                   (187 + shift, 108), (172 + shift, 96), (149 + shift, 101),
                   (126 + shift, 91), (103 + shift, 104), (75 + shift, 97),
                   (64 + shift, 136)]
        art.outlined_polygon(outline, HAIR, 4)
        art.polygon([(67 + shift, 76), (97 + shift, 50), (139 + shift, 43),
                     (128 + shift, 66), (101 + shift, 75)], HAIR_LIGHT)
        art.line([(90 + shift, 52), (124 + shift, 42), (157 + shift, 46)], 2, SKIN_SHADE)
        art.polygon([(160 + shift, 83), (183 + shift, 65), (201 + shift, 91),
                     (192 + shift, 123), (181 + shift, 111)], HAIR)
    elif role == 'eyes':
        side = variant == 'three_quarter'
        eyes = [(112, 123, 12, 15), (155, 123, 12, 15)] if not side else [
            (125, 123, 9, 13), (170, 123, 11, 16)]
        for x, y, rx, ry in eyes:
            art.line([(x - rx - 4, 99), (x, 95), (x + rx + 5, 100)], 3, HAIR)
            art.outlined_ellipse(x, y, rx, ry, SHIRT, 2)
            art.ellipse(x + (3 if side else 0), y + 2, rx * .48, ry * .69, TEAL_DARK)
            art.ellipse(x + (3 if side else 0), y + 3, rx * .28, ry * .52, INK)
            art.ellipse(x - 2 + (3 if side else 0), y - 4, 3, 3, SHIRT)
        art.line([(90 if not side else 105, 145),
                  (112 if not side else 126, 148)], 1.5, SKIN_SHADE)
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
                         (cx + 12, cy), (cx - 12, cy)], SHIRT)
        else:
            rx, ry = {'ee': (31, 9), 'ah': (24, 24), 'oh': (16, 20),
                      'l': (20, 13), 'wide': (34, 17)}[choice]
            art.outlined_ellipse(cx, cy, rx, ry, MOUTH, 3)
            if choice in {'ee', 'ah', 'l', 'wide'}:
                art.ellipse(cx, cy + ry * .58, rx * .55, max(2, ry * .30), TONGUE)
                art.polygon([(cx - rx * .71, cy - ry * .72),
                             (cx + rx * .71, cy - ry * .72),
                             (cx + rx * .56, cy - ry * .18),
                             (cx - rx * .56, cy - ry * .18)], SHIRT)
    elif role.startswith('hand_'):
        sign = 1 if role.endswith('right') else -1
        mirror = lambda x: 128 + sign * (x - 128)
        art.outlined_capsule(128, 92, 128, 133, 15, SKIN)
        art.outlined_ellipse(128, 146, 27, 27, SKIN)
        if variant == 'open':
            for x, length in ((109, 22), (121, 29), (134, 31), (146, 22)):
                art.outlined_capsule(x, 157, x + sign * 2, 157 + length, 7, SKIN, 2)
            art.outlined_capsule(mirror(146), 141, mirror(169), 153, 8, SKIN, 2)
            art.line([(mirror(111), 141), (mirror(143), 143)], 1, SKIN_LIGHT)
        elif variant == 'fist':
            art.outlined_ellipse(128, 162, 28, 24, SKIN)
            for x in (111, 122, 133, 144):
                art.line([(x, 150), (x + 2, 162)], 1.5, SKIN_SHADE)
            art.outlined_capsule(mirror(149), 142, mirror(161), 159, 7, SKIN, 2)
        else:
            art.outlined_capsule(mirror(143), 143, mirror(199), 125, 9, SKIN, 2)
            for x in (113, 125, 137):
                art.outlined_capsule(x, 151, x + 2, 169, 7, SKIN, 2)
            art.outlined_capsule(mirror(145), 132, mirror(162), 153, 7, SKIN, 2)
        art.polygon([(105, 99), (151, 99), (151, 112), (105, 112)], TEAL)
        art.line([(105, 99), (151, 99)], 2, INK)
        art.line([(105, 112), (151, 112)], 2, INK)
    elif role.startswith('upper_arm_'):
        left = role.endswith('left')
        start, end = ((145, 50), (115, 202)) if left else ((111, 50), (141, 202))
        art.outlined_capsule(*start, *end, 28, COAT)
        art.capsule(start[0] - (8 if left else -8), 70,
                    end[0] - (8 if left else -8), 178, 6, COAT_LIGHT)
        art.line([(start[0] - 19, 58), (start[0] + 19, 58)], 2, COAT_DARK)
        art.line([(end[0] - 17, 190), (end[0] + 17, 190)], 2, COAT_DARK)
    elif role.startswith('lower_arm_'):
        left = role.endswith('left')
        start, end = ((142, 44), (121, 207)) if left else ((114, 44), (135, 207))
        art.outlined_capsule(*start, *end, 25, COAT)
        art.capsule(start[0] - (7 if left else -7), 58,
                    end[0] - (7 if left else -7), 165, 5, COAT_LIGHT)
        art.line([(end[0] - 20, 182), (end[0] + 20, 182)], 3, INK)
        art.line([(end[0] - 19, 189), (end[0] + 19, 189)], 5, TEAL)
        art.line([(end[0] - 19, 196), (end[0] + 19, 196)], 2, INK)
    elif role.startswith('upper_leg_'):
        left = role.endswith('left')
        start, end = ((140, 37), (113, 211)) if left else ((116, 37), (143, 211))
        art.outlined_capsule(*start, *end, 29, PANTS)
        art.line([(start[0] - 8, 55), (end[0] - 8, 184)], 5, PANTS_LIGHT)
        art.outlined_ellipse(end[0], 196, 21, 17, PANTS_LIGHT, 2)
    elif role.startswith('lower_leg_'):
        left = role.endswith('left')
        start, end = ((130, 39), (127, 208)) if left else ((126, 39), (129, 208))
        art.outlined_capsule(*start, *end, 25, PANTS)
        art.line([(start[0] - 7, 60), (end[0] - 7, 174)], 4, PANTS_LIGHT)
        art.line([(end[0] - 21, 194), (end[0] + 21, 194)], 2, INK)
    elif role.startswith('foot_'):
        left = role.endswith('left')
        points = [(109, 115), (153, 115), (160, 141), (174, 153), (180, 171),
                  (171, 179), (68, 179), (59, 168), (67, 152), (100, 140)]
        if not left:
            points = [(256 - x, y) for x, y in points]
        art.outlined_polygon(points, INK)
        art.polygon([(min(x for x, _ in points) + 4, 164),
                     (max(x for x, _ in points) - 4, 164),
                     (max(x for x, _ in points) - 8, 176),
                     (min(x for x, _ in points) + 8, 176)], SHIRT)
        for y in (139, 147, 155):
            art.line([(113 if left else 143, y), (140 if left else 116, y + 2)], 1.5, SHIRT)
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
