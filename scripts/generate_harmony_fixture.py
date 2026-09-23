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


def ellipse(x: float, y: float, cx: float, cy: float, rx: float, ry: float) -> bool:
    return ((x - cx) / rx) ** 2 + ((y - cy) / ry) ** 2 <= 1


def capsule(x: float, y: float, ax: float, ay: float, bx: float, by: float, radius: float) -> bool:
    dx, dy = bx - ax, by - ay
    t = max(0.0, min(1.0, ((x - ax) * dx + (y - ay) * dy) / (dx * dx + dy * dy)))
    return math.hypot(x - ax - t * dx, y - ay - t * dy) <= radius


def color_at(role: str, variant: str, x: float, y: float) -> tuple[int, int, int, int]:
    ink = (22, 22, 22, 255)
    paper = (242, 242, 242, 255)
    gray = (155, 155, 155, 255)
    if role == 'torso':
        inside = ellipse(x, y, 128, 128, 65, 88)
        edge = ellipse(x, y, 128, 128, 69, 92)
        return paper if inside else ink if edge else (0, 0, 0, 0)
    if role == 'pelvis':
        inside = ellipse(x, y, 128, 110, 58, 42)
        edge = ellipse(x, y, 128, 110, 62, 46)
        return gray if inside else ink if edge else (0, 0, 0, 0)
    if role == 'head':
        shift = 15 if variant == 'three_quarter' else 0
        inside = ellipse(x, y, 128 + shift, 125, 67, 77)
        edge = ellipse(x, y, 128 + shift, 125, 71, 81)
        return paper if inside else ink if edge else (0, 0, 0, 0)
    if role == 'hair':
        shift = 15 if variant == 'three_quarter' else 0
        return ink if ellipse(x, y, 128 + shift, 74, 69, 31) and y < 91 + 0.13 * (x - 128) else (0, 0, 0, 0)
    if role == 'eyes':
        shift = 18 if variant == 'three_quarter' else 0
        centers = (103 + shift, 150 + shift) if variant == 'front' else (113 + shift, 146 + shift)
        return ink if any(ellipse(x, y, center, 114, 7, 10) for center in centers) else (0, 0, 0, 0)
    if role == 'mouth':
        view, variant = variant.split('__')
        shapes = {'rest': (26, 3), 'mbp': (24, 2), 'fv': (24, 5), 'ee': (37, 5),
                  'ah': (25, 19), 'oh': (15, 17), 'l': (20, 11), 'wide': (39, 12)}
        rx, ry = shapes[variant]
        center = 143 if view == 'three_quarter' else 128
        border = ellipse(x, y, center, 159, rx + 3, ry + 3)
        fill = ellipse(x, y, center, 159, rx, ry)
        return ink if border and not fill else gray if fill else (0, 0, 0, 0)
    if role.startswith('hand_'):
        wrist = capsule(x, y, 128, 93, 128, 126, 15)
        palm = ellipse(x, y, 128, 137, 26, 29)
        finger = variant == 'point' and capsule(x, y, 128, 115, 147, 53, 8)
        thumb = variant == 'open' and capsule(x, y, 145, 135, 166, 114, 8)
        return paper if wrist or palm or finger or thumb else (0, 0, 0, 0)
    if role.startswith('foot_'):
        return ink if ellipse(x, y, 138, 150, 51, 23) else (0, 0, 0, 0)
    if role == 'neck':
        return gray if capsule(x, y, 128, 90, 128, 160, 19) else (0, 0, 0, 0)
    if role.startswith(('upper_arm_', 'lower_arm_')):
        return gray if capsule(x, y, 128, 43, 128, 204, 22) else (0, 0, 0, 0)
    if role.startswith(('upper_leg_', 'lower_leg_')):
        return ink if capsule(x, y, 128, 38, 128, 211, 23) else (0, 0, 0, 0)
    raise ValueError(f'Unknown part role: {role}')


def artwork(role: str, variant: str) -> bytes:
    pixels = bytearray(SIZE * SIZE * 4)
    for y in range(SIZE):
        for x in range(SIZE):
            rgba = color_at(role, variant, x + 0.5, y + 0.5)
            if rgba[3]:
                offset = (y * SIZE + x) * 4
                pixels[offset:offset + 4] = bytes(rgba)
    return rgba_png(pixels)


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
