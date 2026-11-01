#!/usr/bin/env python3
"""Download and verify a released Atari compact FujiNet-NIO library."""
import hashlib
import json
from pathlib import Path, PurePosixPath
import re
import sys
import tempfile
from urllib.request import urlopen
import zipfile


def install(version, destination):
    if not re.fullmatch(r'(0|[1-9]\d*)\.(0|[1-9]\d*)\.(0|[1-9]\d*)', version):
        raise ValueError('version must be MAJOR.MINOR.PATCH')
    destination = Path(destination)
    if destination.exists():
        raise FileExistsError(f'{destination} already exists')
    root = f'https://github.com/markjfisher/fujinet-nio-lib/releases/download/v{version}'
    filename = f'fujinet-nio-lib-atari-compact-{version}.zip'
    with urlopen(f'{root}/SHA256SUMS') as response:
        sums = response.read().decode('ascii').splitlines()
    matches = [line.split() for line in sums if len(line.split()) == 2 and line.split()[1] == filename]
    if len(matches) != 1 or not re.fullmatch(r'[0-9a-fA-F]{64}', matches[0][0]):
        raise ValueError(f'{filename} missing or duplicated in SHA256SUMS')
    with urlopen(f'{root}/{filename}') as response:
        data = response.read()
    if hashlib.sha256(data).hexdigest() != matches[0][0].lower():
        raise ValueError(f'{filename}: checksum mismatch')
    destination.parent.mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory(prefix='.nio-download-', dir=destination.parent) as tmp:
        stage = Path(tmp) / 'package'
        stage.mkdir()
        from io import BytesIO
        with zipfile.ZipFile(BytesIO(data)) as archive:
            for entry in archive.infolist():
                path = PurePosixPath(entry.filename)
                if path.is_absolute() or '..' in path.parts or (entry.external_attr >> 16) & 0o170000 == 0o120000:
                    raise ValueError(f'unsafe archive entry: {entry.filename}')
            manifest = json.loads(archive.read('manifest.json'))
            if manifest.get('version') != version or manifest.get('target') != 'atari-compact':
                raise ValueError('package manifest does not match requested version and target')
            if hashlib.sha256(archive.read('fujinet-nio-atari.lib')).hexdigest() != manifest.get('archive_sha256'):
                raise ValueError('library archive checksum mismatch')
            archive.read('include/fujinet-nio.h')
            archive.extractall(stage)
        stage.rename(destination)
    print(f'Installed {filename} in {destination}')


if __name__ == '__main__':
    if len(sys.argv) != 3:
        sys.exit('usage: fetch_nio_lib.py VERSION DESTINATION')
    try:
        install(sys.argv[1], sys.argv[2])
    except (OSError, ValueError, KeyError, zipfile.BadZipFile) as exc:
        sys.exit(f'FujiNet-NIO download: {exc}')
