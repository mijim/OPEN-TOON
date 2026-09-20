#!/usr/bin/env python3
"""Deploy a local Qt build and audit every bundled Mach-O dependency before signing.

Uses the installed SDK's macdeployqt, then resolves split Homebrew Qt rpaths that
macdeployqt can miss. The output directory must be dedicated to this build.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import plistlib
import re
import shutil
import subprocess

MAGIC = {b'\xcf\xfa\xed\xfe', b'\xfe\xed\xfa\xcf', b'\xca\xfe\xba\xbe', b'\xbe\xba\xfe\xca'}

def run(*args):
    return subprocess.check_output([str(a) for a in args], text=True, stderr=subprocess.STDOUT)

def macho(path):
    if path.is_symlink() or not path.is_file():
        return False
    with path.open('rb') as stream:
        return stream.read(4) in MAGIC

def dependencies(path):
    return [line.strip().split(' (compatibility')[0] for line in run('otool', '-L', path).splitlines()[1:]]

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--build', default='build/locked/open-toon.app')
    parser.add_argument('--output', default='build/release-macos')
    parser.add_argument('--version', required=True)
    parser.add_argument('--reuse-stage', action='store_true')
    args = parser.parse_args()
    repo = Path(__file__).resolve().parents[1]
    output = Path(args.output).resolve()
    app = output / 'OPEN-TOON.app'
    sdk = dict(line.split(':', 1) for line in run('qtpaths', '--query').splitlines())
    prefix = Path(sdk['QT_INSTALL_PREFIX'])
    frameworks = app / 'Contents/Frameworks'
    source_map = {}
    if not args.reuse_stage:
        if app.exists():
            raise SystemExit('Stage already exists; use a fresh output or --reuse-stage for a known deployment.')
        output.mkdir(parents=True, exist_ok=True)
        shutil.copytree(Path(args.build), app, symlinks=True)
        log = subprocess.run(['macdeployqt', str(app), f'-qmldir={repo / "ui"}',
                              f'-qmlimport={sdk["QT_INSTALL_QML"]}', f'-libpath={sdk["QT_INSTALL_LIBS"]}',
                              '-no-codesign', '-verbose=2'], text=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
        (output / 'deployment.log').write_text(log.stdout)
    logs = [output / 'deployment.log', repo / 'build/macos-deploy.log']
    for log in logs:
        if log.exists():
            lines = log.read_text().splitlines()
            for i, line in enumerate(lines[:-1]):
                match = re.search(r'copied: "([^"]+)"', line)
                target = re.search(r'to "([^"]+)"', lines[i + 1])
                if match and target:
                    source_map[str(Path(target[1]).resolve())] = str(Path(match[1]).resolve())
    for path in app.rglob('*'):
        if not path.is_symlink():
            path.chmod(path.stat().st_mode | 0o200)
    frameworks.mkdir(exist_ok=True)

    def copy_dependency(source, relative):
        source = source.resolve()
        target = frameworks / relative
        if '.framework/' in str(relative):
            name = str(relative).split('/')[0]
            root = source
            while root.name != name and root != root.parent:
                root = root.parent
            if root.name != name:
                raise RuntimeError(f'Framework source is unresolved: {relative}')
            destination = frameworks / name
            if not destination.exists():
                shutil.copytree(root, destination, symlinks=True,
                                ignore=shutil.ignore_patterns('Headers', 'Modules', '*.dSYM', '.DS_Store'))
                for path in destination.rglob('*'):
                    if not path.is_symlink():
                        path.chmod(path.stat().st_mode | 0o200)
                        if path.is_file():
                            source_map[str(path.resolve())] = str(root / path.relative_to(destination))
        elif not target.exists():
            shutil.copy2(source, target)
            target.chmod(target.stat().st_mode | 0o200)
        source_map[str(target.resolve())] = str(source)
        return target

    processed = set()
    while True:
        pending = [p for p in app.rglob('*') if macho(p) and str(p) not in processed]
        if not pending:
            break
        for binary in pending:
            processed.add(str(binary))
            identity = run('otool', '-D', binary).splitlines()[1:]
            for dep in dependencies(binary):
                if identity and dep == identity[0]:
                    continue
                if dep.startswith(('/usr/lib/', '/System/Library/')):
                    continue
                replacement = None
                if dep.startswith('@rpath/'):
                    relative = dep[len('@rpath/'):]
                    target = frameworks / relative
                    if not target.exists():
                        source = Path(sdk['QT_INSTALL_LIBS']) / relative
                        if not source.exists():
                            raise RuntimeError(f'Cannot resolve bundled dependency {dep}')
                        copy_dependency(source, relative)
                elif dep.startswith(str(prefix)):
                    source = Path(dep)
                    relative = dep[dep.index('/Qt') + 1:] if '.framework/' in dep else source.name
                    copy_dependency(source, relative)
                    replacement = '@rpath/' + relative
                elif dep.startswith('@executable_path/'):
                    target = app / 'Contents/MacOS' / dep[len('@executable_path/'):]
                    if not target.exists():
                        raise RuntimeError(f'Missing executable-relative dependency: {dep}')
                elif dep.startswith('@loader_path/'):
                    target = binary.parent / dep[len('@loader_path/'):]
                    if not target.exists():
                        raise RuntimeError(f'Missing loader-relative dependency: {dep}')
                else:
                    raise RuntimeError(f'External dependency is not redistributable from this SDK: {dep}')
                if replacement:
                    run('install_name_tool', '-change', dep, replacement, binary)
            if identity and binary.is_relative_to(frameworks):
                run('install_name_tool', '-id', '@rpath/' + str(binary.relative_to(frameworks)), binary)
            # Remove absolute developer-machine search paths. Bundled Qt paths suffice.
            loads = run('otool', '-l', binary)
            for rpath in re.findall(r'cmd LC_RPATH\s+cmdsize \d+\s+path (.*?) \(offset', loads):
                if rpath.startswith('/'):
                    run('install_name_tool', '-delete_rpath', rpath, binary)
    main_binary = app / 'Contents/MacOS/open-toon'
    if '@executable_path/../Frameworks' not in run('otool', '-l', main_binary):
        run('install_name_tool', '-add_rpath', '@executable_path/../Frameworks', main_binary)
    inventory = []
    kegs = set()
    minimum = (0, 0)
    for binary in app.rglob('*'):
        if not macho(binary):
            continue
        deps = dependencies(binary)
        for dep in deps:
            if dep.startswith('/') and not dep.startswith(('/usr/lib/', '/System/Library/')):
                raise RuntimeError(f'Unresolved external library: {dep}')
            if dep.startswith('@rpath/') and not (frameworks / dep[7:]).exists():
                # Plugin own install IDs need not be in Frameworks.
                identity = run('otool', '-D', binary).splitlines()[1:]
                if not identity or dep != identity[0]:
                    raise RuntimeError(f'Unresolved bundled rpath: {dep}')
        metadata = run('xcrun', 'vtool', '-show-build', binary)
        for version in re.findall(r'minos (\d+(?:\.\d+)*)', metadata):
            minimum = max(minimum, tuple(map(int, version.split('.'))))
        source = source_map.get(str(binary.resolve()))
        if source and '/Cellar/' in source:
            keg = Path(source.split('/Cellar/')[0] + '/Cellar').joinpath(*source.split('/Cellar/')[1].split('/')[:2])
            kegs.add(keg)
        inventory.append({'path': str(binary.relative_to(app)), 'dependencies': deps})
    with (app / 'Contents/Info.plist').open('rb') as stream:
        info = plistlib.load(stream)
    info.update(CFBundleName='OPEN-TOON', CFBundleShortVersionString=args.version,
                CFBundleVersion=args.version, LSMinimumSystemVersion='.'.join(map(str, minimum)))
    with (app / 'Contents/Info.plist').open('wb') as stream:
        plistlib.dump(info, stream)
    (output / 'source-map.local.json').write_text(json.dumps(source_map, indent=2))
    (output / 'kegs.local.json').write_text(json.dumps(sorted(map(str, kegs)), indent=2))
    (output / 'runtime-manifest.json').write_text(json.dumps({'version': args.version, 'architecture': 'arm64',
        'minimum_macos': '.'.join(map(str, minimum)), 'libraries': inventory}, indent=2) + '\n')
    print(f'Audited {len(inventory)} Mach-O files; minimum macOS {info["LSMinimumSystemVersion"]}.')
    print('Add notices and corresponding source inventory before final signing and archiving.')

if __name__ == '__main__':
    main()
