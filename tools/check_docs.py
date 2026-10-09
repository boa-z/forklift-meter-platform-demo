#!/usr/bin/env python3
"""检查 Product 文档的本地链接，以及图集里的截图都是存在的 800×480 PNG、没有多余文件。"""
from pathlib import Path
import re
import struct

ROOT = Path(__file__).resolve().parents[1]
LINK = re.compile(r'!?\[[^\]]*\]\(([^)\s]+)\)')


def main():
    documents = [ROOT / 'README.md', ROOT / 'README.zh-CN.md', *(ROOT / 'docs').rglob('*.md')]
    for path in documents:
        for target in LINK.findall(path.read_text(encoding='utf-8')):
            if re.match(r'(?:[a-zA-Z][a-zA-Z0-9+.-]*:|#)', target):
                continue
            resolved = (path.parent / target.split('#', 1)[0]).resolve()
            if not resolved.is_relative_to(ROOT) or not resolved.exists():
                raise SystemExit('Missing or non-Product link: ' + str(path.relative_to(ROOT)) + ': ' + target)
    gallery = ROOT / 'docs/ui/screenshots.md'
    shown = {Path(t).name for t in LINK.findall(gallery.read_text(encoding='utf-8')) if t.endswith('.png')}
    stored = {p.name for p in (ROOT / 'docs/ui/screenshots').glob('*.png')}
    if shown != stored:
        raise SystemExit('Gallery and screenshot folder differ: ' + ', '.join(sorted(shown ^ stored)))
    for name in sorted(stored):
        raw = (ROOT / 'docs/ui/screenshots' / name).read_bytes()
        if raw[:8] != b'\x89PNG\r\n\x1a\n' or struct.unpack('>II', raw[16:24]) != (800, 480):
            raise SystemExit('Not an 800x480 PNG: ' + name)
    print('Product documentation links and %d gallery screenshots PASS' % len(stored))


if __name__ == '__main__':
    main()
