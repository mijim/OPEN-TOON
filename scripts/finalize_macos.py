#!/usr/bin/env python3
"""Ad-hoc sign and archive a dependency-audited macOS preview, with source assets."""
import argparse
import hashlib
import json
from pathlib import Path
import shutil
import subprocess
import tarfile
from package_macos import macho

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--stage', default='build/release-macos')
parser.add_argument('--version', required=True)
args = parser.parse_args()
stage = Path(args.stage).resolve()
app = stage / 'OPEN-TOON.app'
notices = app / 'Contents/Resources/ThirdParty'
sources = stage / 'dependency-sources'
for required in (notices / 'NOTICE.txt', sources / 'sources.json', stage / 'runtime-manifest.json'):
    if not required.is_file():
        raise SystemExit(f'Missing release input: {required.name}')
for entry in json.loads((sources / 'sources.json').read_text()):
    source = sources / entry['archive']
    if hashlib.file_digest(source.open('rb'), 'sha256').hexdigest() != entry['sha256']:
        raise SystemExit(f'Source hash mismatch: {entry["name"]}')
for binary in app.rglob('*'):
    if macho(binary):
        subprocess.run(['codesign', '--force', '--sign', '-', '--timestamp=none', str(binary)], check=True)
for framework in sorted(app.rglob('*.framework'), key=lambda p: len(p.parts), reverse=True):
    subprocess.run(['codesign', '--force', '--sign', '-', '--timestamp=none', str(framework)], check=True)
subprocess.run(['codesign', '--force', '--sign', '-', '--timestamp=none', str(app)], check=True)
subprocess.run(['codesign', '--verify', '--deep', '--strict', str(app)], check=True)
archive = stage / f'OPEN-TOON-{args.version}-macOS-arm64.zip'
subprocess.run(['ditto', '-c', '-k', '--sequesterRsrc', '--keepParent', str(app), str(archive)], check=True)
source_archive = stage / f'OPEN-TOON-{args.version}-dependency-sources.tar.gz'
with tarfile.open(source_archive, 'w:gz') as tar:
    def metadata(info):
        info.uid = info.gid = 0
        info.uname = info.gname = ''
        return info
    tar.add(sources, arcname='dependency-sources', filter=metadata)
manifest = stage / 'SHA256SUMS.txt'
manifest.write_text(''.join(f'{hashlib.file_digest(p.open("rb"), "sha256").hexdigest()}  {p.name}\n'
                            for p in (archive, source_archive, stage / 'runtime-manifest.json')))
print(manifest.read_text())
print('Ad-hoc signed preview only; Apple notarization and Developer ID signing are not claimed.')
