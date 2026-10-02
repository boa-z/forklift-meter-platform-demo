#!/usr/bin/env python3
"""检查 Product 文档链接、截图尺寸、哈希及源码对应关系。"""
import json
from pathlib import Path
import re
import struct
from capture_screenshots import ROOT, digest, source_digest

# 图集用例数，必须与 tools/capture_screenshots.py 里的 cases 数量一致。
EXPECTED_CAPTURES = 18


def main():
    documents = [ROOT / 'README.md', ROOT / 'README.zh-CN.md', *(ROOT / 'docs').rglob('*.md')]
    for path in documents:
        for target in re.findall(r'!?\[[^\]]*\]\(([^)\s]+)\)', path.read_text(encoding='utf-8')):
            if re.match(r'(?:[a-zA-Z][a-zA-Z0-9+.-]*:|#)', target):
                continue
            resolved = (path.parent / target.split('#', 1)[0]).resolve()
            if not resolved.is_relative_to(ROOT) or not resolved.exists():
                raise SystemExit('Missing or non-Product link: ' + str(path.relative_to(ROOT)) + ': ' + target)
    folder = ROOT / 'docs/ui/screenshots'
    manifest = json.loads((folder / 'manifest.json').read_text(encoding='utf-8'))
    if manifest['product_source_sha256'] != source_digest(ROOT):
        raise SystemExit('Product source changed: rebuild and regenerate SDL2 screenshots')
    cases = manifest['screenshots']
    if len(cases) != EXPECTED_CAPTURES or len({case['file'] for case in cases}) != EXPECTED_CAPTURES:
        raise SystemExit('Expected %d unique documented SDL2 captures' % EXPECTED_CAPTURES)
    for case in cases:
        path = folder / case['file']
        raw = path.read_bytes()
        if raw[:8] != bytes((137,80,78,71,13,10,26,10)) or struct.unpack('>II', raw[16:24]) != (800,480):
            raise SystemExit('Invalid PNG dimensions: ' + case['file'])
        if digest(path) != case['sha256'] or case['report']['result'] != 'PASS':
            raise SystemExit('Invalid screenshot evidence: ' + case['file'])
    print('Product documentation links and %d source-bound SDL2 screenshots PASS' % EXPECTED_CAPTURES)


if __name__ == '__main__':
    main()
