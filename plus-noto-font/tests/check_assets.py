#!/usr/bin/env python3
# SPDX-License-Identifier: MIT
"""Verify all shipped assets; optionally compare to the accepted test font."""
import argparse
import json
from pathlib import Path
import struct
import sys

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / 'scripts'))
from font_format import sha256
from prepare_font import read_font


def reference_specs(path):
    data = path.read_bytes()
    if sha256(data) != '2eab0943d6a0120e8b4da6330e0d761cdfd6852272116a601b7cbb97af6a624c':
        raise ValueError('Not the accepted 1.0.0-plus.1 reference font')
    _, _, _, ns, _, _, so, go, _, _, _ = struct.unpack_from('<4sHHIIIIIIII', data)
    result = {}
    for n in range(ns):
        px, weight, threshold, _, ascent, descent, line_height, _, first, count = struct.unpack_from('<HHHHhhhhII', data, so + n * 24)
        if threshold != 112:
            continue
        glyphs = {}
        for i in range(first, first + count):
            cp, offset, width, height, left, top, advance, _ = struct.unpack_from('<IIHHhhhH', data, go + i * 20)
            bitmap = data[offset:offset + (width * height + 7) // 8]
            glyphs[cp] = (cp, width, height, left, top, advance, bitmap)
        result[(px, weight)] = ((ascent, descent, line_height), glyphs)
    return result


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--output', type=Path, required=True)
    p.add_argument('--reference', type=Path, help='Optional accepted NTF test pack, not shipped here')
    args = p.parse_args()
    args.output.mkdir(parents=True, exist_ok=True)
    assets = ROOT / 'assets/fonts'
    manifest = json.loads((assets / 'fonts-manifest.json').read_text('utf-8'))
    charset_data = (assets / manifest['charset_file']).read_bytes()
    assert sha256(charset_data) == manifest['charset_sha256']
    codes = set(map(ord, charset_data.decode('utf-8')))
    assert len(codes) == 7056
    expected = {(px, w) for px in (24, 28, 32, 36, 40, 48) for w in (400, 500, 600)}
    assert {(f['pixels'], f['weight']) for f in manifest['fonts']} == expected
    ref = reference_specs(args.reference) if args.reference else None
    report = {'status': 'PASS', 'fonts': [], 'glyph_records': 0,
              'reference_glyph_comparisons': 0, 'reference_checked': bool(ref)}
    for px, weight in sorted(expected):
        data, spec, glyphs, _, entry = read_font(assets, px, weight)
        glyph_map = {g[0]: g for g in glyphs}
        assert set(glyph_map) == codes
        if ref:
            old_metrics, old = ref[(px, weight)]
            assert tuple(spec[3:]) == old_metrics
            for cp, glyph in old.items():
                assert glyph_map[cp] == glyph, (px, weight, cp)
            report['reference_glyph_comparisons'] += len(old)
        filename = 'font-%d-%d.ntf' % (px, weight)
        (args.output / filename).write_bytes(data)
        report['fonts'].append(dict(file=filename, pixels=px, weight=weight,
                                   bytes=len(data), glyphs=len(glyphs), sha256=entry['sha256']))
        report['glyph_records'] += len(glyphs)
    (args.output / 'asset-check.json').write_text(json.dumps(report, ensure_ascii=False, indent=2) + '\n', encoding='utf-8')
    print('PASS %d fonts; %d glyph records; %d reference glyph/metric matches' %
          (len(report['fonts']), report['glyph_records'], report['reference_glyph_comparisons']))


if __name__ == '__main__':
    main()
