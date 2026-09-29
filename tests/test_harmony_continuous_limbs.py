"""Check original HM-06 limb sprites remain deterministic, opaque and continuous."""
import json
from pathlib import Path
import struct
import subprocess
import sys
import tempfile
import unittest
import zlib

ROOT = Path(__file__).resolve().parents[1]
FIXTURE = ROOT / 'tests/fixtures/harmony-continuous-limbs'


def rgba(path: Path) -> bytes:
    data = path.read_bytes()
    assert data[:8] == b'\x89PNG\r\n\x1a\n'
    assert struct.unpack_from('>II', data, 16) == (512, 512)
    compressed = bytearray()
    offset = 8
    while offset < len(data):
        length = struct.unpack_from('>I', data, offset)[0]
        kind = data[offset + 4:offset + 8]
        if kind == b'IDAT':
            compressed.extend(data[offset + 8:offset + 8 + length])
        offset += length + 12
    raw = zlib.decompress(compressed)
    assert len(raw) == 512 * (1 + 512 * 4)
    return b''.join(raw[y * 2049 + 1:(y + 1) * 2049] for y in range(512))


class ContinuousLimbFixtureTest(unittest.TestCase):
    def test_registered_sprites_have_one_connected_silhouette(self):
        spec = json.loads((FIXTURE / 'rig.json').read_text(encoding='utf-8'))
        self.assertEqual(set(spec['limbs']),
                         {'arm_left', 'arm_right', 'leg_left', 'leg_right'})
        with tempfile.TemporaryDirectory() as temporary:
            generated = Path(temporary)
            subprocess.run([sys.executable, str(ROOT / 'scripts/generate_harmony_continuous_limbs.py'),
                            '--output', str(generated)], check=True)
            parts = dict(spec['limbs'])
            parts['pelvis'] = {**spec['joined_waist'], 'joints_scene_px': []}
            for role, limb in parts.items():
                name = f'{role}__base.png'
                self.assertEqual((generated / name).read_bytes(),
                                 (FIXTURE / 'parts' / name).read_bytes())
                pixels = rgba(generated / name)
                opaque = {i for i in range(512 * 512) if pixels[i * 4 + 3]}
                self.assertGreater(len(opaque), 10000)
                center_x, center_y = limb['center_px']
                for x, y in limb['joints_scene_px']:
                    local = (y - center_y + 256) * 512 + (x - center_x + 256)
                    self.assertIn(local, opaque, (role, x, y))
                visited = {next(iter(opaque))}
                pending = list(visited)
                while pending:
                    current = pending.pop()
                    x, y = current % 512, current // 512
                    for neighbor in ((current - 1 if x else current),
                                     (current + 1 if x < 511 else current),
                                     (current - 512 if y else current),
                                     (current + 512 if y < 511 else current)):
                        if neighbor in opaque and neighbor not in visited:
                            visited.add(neighbor)
                            pending.append(neighbor)
                self.assertEqual(visited, opaque, role)


if __name__ == '__main__':
    unittest.main()
