import hashlib
import importlib.util
import io
import json
from pathlib import Path
import tempfile
import unittest
from unittest.mock import patch
import zipfile

spec = importlib.util.spec_from_file_location('fetch_nio_lib', Path(__file__).resolve().parents[1] / 'tools/fetch_nio_lib.py')
fetch = importlib.util.module_from_spec(spec)
spec.loader.exec_module(fetch)


def package(extra=None):
    data = io.BytesIO()
    lib = b'an Atari library'
    with zipfile.ZipFile(data, 'w') as z:
        z.writestr('fujinet-nio-atari.lib', lib)
        z.writestr('include/fujinet-nio.h', 'header')
        z.writestr('manifest.json', json.dumps({'version': '1.2.3', 'target': 'atari-compact',
            'archive_sha256': hashlib.sha256(lib).hexdigest()}))
        if extra:
            z.writestr(extra, 'bad')
    return data.getvalue()


class FetchTest(unittest.TestCase):
    def fetch_fixture(self, data, checksum=None):
        digest = checksum or hashlib.sha256(data).hexdigest()
        sums = f'{digest}  fujinet-nio-lib-atari-compact-1.2.3.zip\n'.encode()
        return patch.object(fetch, 'urlopen', side_effect=lambda url: io.BytesIO(sums if url.endswith('SHA256SUMS') else data))

    def test_installs_verified_package(self):
        with tempfile.TemporaryDirectory() as tmp, self.fetch_fixture(package()):
            dest = Path(tmp) / 'atari-1.2.3'
            fetch.install('1.2.3', dest)
            self.assertEqual((dest / 'fujinet-nio-atari.lib').read_bytes(), b'an Atari library')
            self.assertTrue((dest / 'include/fujinet-nio.h').is_file())
            with self.assertRaises(FileExistsError):
                fetch.install('1.2.3', dest)

    def test_rejects_bad_checksum_without_installing(self):
        with tempfile.TemporaryDirectory() as tmp, self.fetch_fixture(package(), '0' * 64):
            dest = Path(tmp) / 'package'
            with self.assertRaisesRegex(ValueError, 'checksum mismatch'):
                fetch.install('1.2.3', dest)
            self.assertFalse(dest.exists())

    def test_rejects_unsafe_archive_without_installing(self):
        with tempfile.TemporaryDirectory() as tmp, self.fetch_fixture(package('../escape')):
            dest = Path(tmp) / 'package'
            with self.assertRaisesRegex(ValueError, 'unsafe archive'):
                fetch.install('1.2.3', dest)
            self.assertFalse(dest.exists())

    def test_rejects_invalid_version(self):
        with self.assertRaises(ValueError):
            fetch.install('../bad', '/tmp/ignored')


if __name__ == '__main__':
    unittest.main()
