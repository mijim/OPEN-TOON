"""Verify the reproducible HM-00 input assets and exact timeline profile."""
from fractions import Fraction
import hashlib
import json
from pathlib import Path
import struct
import subprocess
import sys
import tempfile
import unittest
import wave
import zlib


ROOT = Path(__file__).resolve().parents[1]
SPEC = ROOT / 'tests/fixtures/harmony-moment/shot.json'


class HarmonyFixtureTest(unittest.TestCase):
    def test_profile_and_generator(self):
        shot = json.loads(SPEC.read_text(encoding='utf-8'))
        self.assertEqual(shot['canvas'], {'width': 1920, 'height': 1080, 'pixel_aspect': [1, 1]})
        self.assertEqual(len(shot['part_roles']), 19)
        self.assertEqual(len(set(shot['part_roles'])), 19)
        self.assertEqual(len(shot['substitutions']['mouth']), 8)
        self.assertEqual(len(shot['substitutions']['hand_right']), 3)
        self.assertEqual(len(shot['intended_rig']['published_controls']), 12)
        self.assertEqual(shot['shot_beats'][0]['range'][0], 0)
        self.assertEqual(shot['shot_beats'][-1]['range'][1], 480)
        self.assertTrue(all(a['range'][1] == b['range'][0]
                            for a, b in zip(shot['shot_beats'], shot['shot_beats'][1:])))
        for variant in shot['time_variants']:
            numerator, denominator = variant['fps']
            exact_samples = Fraction(variant['frames'] * denominator * variant['sample_rate'], numerator)
            self.assertEqual(exact_samples.denominator, 1)
            self.assertEqual(variant['audio_samples'], exact_samples.numerator)
        self.assertEqual(shot['time_variants'][0]['audio_samples'], 960000)
        self.assertEqual(shot['time_variants'][1]['audio_samples'], 960960)

        with tempfile.TemporaryDirectory() as directory:
            output = Path(directory)
            subprocess.run([sys.executable, str(ROOT / 'scripts/generate_harmony_fixture.py'),
                            '--output', str(output), '--long-audio', '--previews'], check=True, capture_output=True)
            provenance = json.loads((output / 'PROVENANCE.json').read_text(encoding='utf-8'))
            self.assertEqual(provenance['license'], 'GPL-3.0-or-later')
            for name, digest in provenance['files_sha256'].items():
                self.assertEqual(hashlib.sha256((output / name).read_bytes()).hexdigest(), digest)
            expected_parts = len(shot['part_roles']) + 15 + 4 + 3
            self.assertEqual(len(list((output / 'parts').glob('*.png'))), expected_parts)
            for part in (output / 'parts').glob('*.png'):
                self.assertEqual(part.read_bytes(), (SPEC.parent / 'parts' / part.name).read_bytes())
            self.assertEqual(set(shot['reference_centers_px']), set(shot['part_roles']))
            self.assertEqual(set(shot['reference_paint_order']), set(shot['part_roles']))
            for pose in shot['reference_key_poses']:
                image = (output / f"reference_{pose['frame']:04d}.png").read_bytes()
                self.assertEqual(struct.unpack_from('>II', image, 16), (1920, 1080))
                committed = SPEC.parent / f"reference_{pose['frame']:04d}.png"
                self.assertEqual(image, committed.read_bytes())
            front = output / 'parts/mouth__front__ah.png'
            quarter = output / 'parts/mouth__three_quarter__ah.png'
            self.assertNotEqual(front.read_bytes(), quarter.read_bytes())
            data = front.read_bytes()
            self.assertEqual(data[:8], b'\x89PNG\r\n\x1a\n')
            self.assertEqual(struct.unpack_from('>II', data, 16), (256, 256))
            self.assertEqual(data[25], 6)  # RGBA, with alpha rather than an opaque matte.
            self.assertEqual(data[37:41], b'sRGB')
            compressed = bytearray()
            pos = 8
            while pos < len(data):
                length = struct.unpack_from('>I', data, pos)[0]
                kind = data[pos + 4:pos + 8]
                if kind == b'IDAT':
                    compressed.extend(data[pos + 8:pos + 8 + length])
                pos += length + 12
            pixels = zlib.decompress(compressed)
            self.assertEqual(len(pixels), 256 * (1 + 256 * 4))
            self.assertEqual(pixels[1:5], b'\0\0\0\0')
            self.assertNotEqual(pixels[159 * (1 + 256 * 4) + 1 + 128 * 4 + 3], 0)

            for clip in (*shot['audio']['clips'], shot['audio']['fractional_dialogue_clip']):
                with wave.open(str(output / clip), 'rb') as audio:
                    self.assertEqual(audio.getparams()[:3], (1, 2, 48000))
                    self.assertEqual(audio.getnframes(), 960960)
            with wave.open(str(output / 'dialogue_reference_24.wav'), 'rb') as audio:
                for frame in shot['audio']['cue_frames']:
                    audio.setpos(frame * 2000 + 1)
                    self.assertNotEqual(audio.readframes(1), b'\0\0')
            with wave.open(str(output / 'dialogue_reference_24000_1001.wav'), 'rb') as audio:
                for frame in shot['audio']['cue_frames']:
                    audio.setpos(frame * 2002 + 1)
                    self.assertNotEqual(audio.readframes(1), b'\0\0')
            with wave.open(str(output / 'drift_10_minutes.wav'), 'rb') as audio:
                self.assertEqual(audio.getnframes(), 600 * 48000)
                for second in range(0, 600, 30):
                    audio.setpos(second * 48000 + 1)
                    self.assertNotEqual(audio.readframes(1), b'\0\0')

    def test_ten_minute_frame_boundary(self):
        sample_rate = 48000
        for rate in (Fraction(24, 1), Fraction(24000, 1001)):
            frame = round(600 * rate)
            exact_sample = Fraction(frame * sample_rate, 1) / rate
            floor_sample = exact_sample.numerator // exact_sample.denominator
            self.assertLess(exact_sample - floor_sample, 1)
            self.assertLess(abs(Fraction(floor_sample, sample_rate) - 600), 1 / rate)


if __name__ == '__main__':
    unittest.main()
