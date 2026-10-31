#!/usr/bin/env python3
# SPDX-License-Identifier: MIT
"""Package this component without local builds, caches, or external projects."""
import argparse
import hashlib
from pathlib import Path
import zipfile

ROOT = Path(__file__).resolve().parents[1]
FILES = ('README.md', 'SKILL.md', 'LICENSE', '验证报告.json')
DIRS = ('components', 'assets', 'scripts', 'licenses', 'examples', 'tests', 'agents')


def digest(data):
    return hashlib.sha256(data).hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, default=ROOT / '交付/plus-noto-font-1.0.0.zip')
    args = parser.parse_args()
    selected = [ROOT / f for f in FILES]
    for folder in DIRS:
        selected.extend(p for p in (ROOT / folder).rglob('*') if p.is_file()
                        and '__pycache__' not in p.parts and p.suffix != '.pyc')
    payload = {p.relative_to(ROOT).as_posix(): p.read_bytes() for p in sorted(selected)}
    for required in ('SKILL.md', 'README.md', 'LICENSE', 'licenses/OFL.txt',
                     'components/plus_font/plus_font.cc', 'assets/fonts/fonts-manifest.json'):
        if required not in payload:
            raise ValueError('Required package entry missing: ' + required)
    sums = ''.join(digest(data) + '  ' + name + '\n' for name, data in payload.items())
    payload['SHA256SUMS.txt'] = sums.encode('utf-8')
    args.output.parent.mkdir(parents=True, exist_ok=True)
    with zipfile.ZipFile(args.output, 'w') as z:
        for name, data in payload.items():
            info = zipfile.ZipInfo('plus-noto-font/' + name, (2026, 10, 31, 0, 0, 0))
            info.compress_type = zipfile.ZIP_STORED if name.endswith('.zlib') else zipfile.ZIP_DEFLATED
            info.external_attr = (0o100644 << 16)
            z.writestr(info, data, compresslevel=9)
    with zipfile.ZipFile(args.output) as z:
        if z.testzip() is not None:
            raise ValueError('Archive CRC failure')
        for name, data in payload.items():
            if z.read('plus-noto-font/' + name) != data:
                raise ValueError('Archive content differs: ' + name)
    checksum = digest(args.output.read_bytes())
    args.output.with_suffix('.sha256').write_text(checksum + '  ' + args.output.name + '\n', encoding='utf-8')
    print('PASS %d files; %d bytes; SHA256=%s' % (len(payload), args.output.stat().st_size, checksum))


if __name__ == '__main__':
    main()
