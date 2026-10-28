#!/usr/bin/env python3
"""Build the Product's reviewed CJK subsets from the OTF it keeps under assets/fonts.

The Product owns its font files: the source OTF/TTF, its license text and the generated C live in the
Product repository. Normal builds and --check need no Node/network; only regeneration runs lv_font_conv.
"""
import argparse
import hashlib
import json
import os
import re
import shutil
import subprocess
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
FONT = 'assets/fonts/SourceHanSansSC-Normal.otf'
LICENSE = 'assets/LICENSES/SourceHanSansSC-OFL.txt'
ORIGIN = ('LVGL v9.6.0 80ca777e37a2b176770726a02e07a6fb79ef0b39, '
          'scripts/generators/built_in_font/SourceHanSansSC-Normal.otf')
VERSION = '1.5.3'


def portable_font_source(text):
    """规范生成注释中的字体路径，避免暴露本机目录并保持跨平台哈希。"""
    return re.sub(r'(?m)( \* Opts: --font ).*?( --size )', lambda match: match[1] + FONT + match[2], text)


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def product_font_text(root, config):
    """合并显式声明的产品文字来源，避免动态目录中的字符漏出字体子集。"""
    extra = config.get('text_sources', [])
    if not isinstance(extra, list) or any(not isinstance(item, str) for item in extra):
        raise ValueError('font-config text_sources must be a list of relative paths')
    names = [config.get('translations', 'ui/demo_i18n.c'), *extra]
    root = root.resolve()
    texts = []
    for name in dict.fromkeys(names):
        path = (root / name).resolve()
        if Path(name).is_absolute() or not path.is_relative_to(root):
            raise ValueError('font text source must remain inside the Product: ' + name)
        texts.append(path.read_text(encoding='utf-8'))
    return '\n'.join(texts)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--check', action='store_true')
    parser.add_argument('--product-root', type=Path, default=ROOT)
    args = parser.parse_args()
    product = args.product_root.resolve()
    config_path = product / 'assets/font-config.json'
    config = json.loads(config_path.read_text(encoding='utf-8')) if config_path.exists() else {}
    # 只从可执行字符串和翻译表提取字符，中文说明性注释不应改变字体资产。
    text = re.sub(r'/\*.*?\*/|//[^\r\n]*', '', product_font_text(product, config), flags=re.S)
    symbols = ''.join(sorted({c for c in text if ord(c) > 127}))
    manifest_path = product / 'assets/fonts.json'
    metadata = dict(name=config.get('name', 'Meter Demo CJK'), upstream='Source Han Sans SC', source=FONT,
                    origin=ORIGIN, source_sha256=digest(product / FONT), license='OFL-1.1',
                    license_file=LICENSE, converter='lv_font_conv@' + VERSION, symbols=symbols,
                    modifications='Renamed subset; ASCII and demo translations; 14/20 px, 4 bpp', files={})
    if args.check:
        saved = json.loads(manifest_path.read_text(encoding='utf-8'))
        metadata['files'] = saved['files']
        assert metadata == saved, 'Font source or translations changed; regenerate fonts'
        assert (product / LICENSE).stat().st_size > 500, 'font license text missing'
        for name, sha in saved['files'].items():
            assert digest(product / name) == sha, 'Generated font hash mismatch: ' + name
        assert len(saved['files']) == 2
        print('Chinese font subset, source, license and hashes PASS')
        return
    npm = shutil.which('npm.cmd' if os.name == 'nt' else 'npm')
    if not npm:
        raise SystemExit('Node.js/npm required only to regenerate fonts')
    for size in (14, 20):
        filename = f"generated/{config.get('prefix', 'meter_demo_cjk')}_{size}.c"
        subprocess.run([npm, 'exec', '--yes', '--package=lv_font_conv@' + VERSION, '--',
                        'lv_font_conv', '--font', FONT, '--size', str(size), '--bpp', '4',
                        '--format', 'lvgl', '--no-compress', '--no-prefilter',
                        '--range', '0x20-0x7e', '--symbols', symbols, '--output', filename],
                       cwd=product, check=True)
        # Normalize tool output for cross-platform hashes.
        out = product / filename
        out.write_text(portable_font_source(out.read_text(encoding='utf-8')).rstrip() + chr(10), encoding='utf-8', newline='\n')
        metadata['files'][filename] = digest(out)
    manifest_path.write_text(json.dumps(metadata, ensure_ascii=False, indent=2) + '\n', encoding='utf-8', newline='\n')


if __name__ == '__main__':
    main()
