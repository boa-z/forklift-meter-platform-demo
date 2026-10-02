#!/usr/bin/env python3
"""从真实 SDL2 帧导出 Product 文档截图，保留命令、源码和像素证据。"""
import argparse
from datetime import datetime, timezone
import hashlib
import json
import os
from pathlib import Path
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]
SOURCE_DIRS = ('application', 'assets', 'catalog', 'generated', 'product',
               'protocol', 'services', 'sim', 'ui')
# 摘要只覆盖源码：对象文件与字节码进出源码树不应改变证据，否则构建残留会伪装成源码变更。
BUILD_ARTIFACT_SUFFIXES = ('.o', '.a', '.pyc')
BUILD_ARTIFACT_DIRS = ('__pycache__', '.pytest_cache')


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def source_digest(root, directories=SOURCE_DIRS):
    result = hashlib.sha256()
    # Path 在 Windows 按大小写不敏感排序；摘要统一使用 POSIX 字符串顺序。
    files = (p for name in directories for p in (root / name).rglob('*')
             if p.is_file() and p.suffix not in BUILD_ARTIFACT_SUFFIXES
             and not any(part in BUILD_ARTIFACT_DIRS for part in p.parts))
    for path in sorted(files, key=lambda p: p.relative_to(root).as_posix()):
        result.update(path.relative_to(root).as_posix().encode('utf-8') + b'\0')
        result.update(path.read_bytes())
    return result.hexdigest()


def revision(root):
    return subprocess.check_output(['git', '-C', str(root), 'rev-parse', 'HEAD'], text=True).strip()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--framework-root', type=Path, required=True)
    parser.add_argument('--binary', type=Path, required=True)
    parser.add_argument('--output', type=Path, default=ROOT / 'docs/ui/screenshots')
    args = parser.parse_args()
    from PIL import Image
    framework, binary, output = args.framework_root.resolve(), args.binary.resolve(), args.output.resolve()
    output.mkdir(parents=True, exist_ok=True)
    cases = []
    for language, code in (('english', 'en'), ('chinese', 'zh-CN')):
        for name, page, subpage in (('dashboard', 0, 0), ('monitor', 1, 0),
                                    ('monitor-2', 1, 1), ('faults', 2, 0),
                                    ('settings', 3, 0), ('settings-clock', 3, 1)):
            cases.append((name + '-' + code, language, page, subpage, ['--visual', 'mid']))
    for scenario in ('warning', 'stale', 'unknown', 'error', 'offline'):
        cases.append(('dashboard-' + scenario + '-zh-CN', 'chinese', 0, 0, ['--scenario', scenario]))
    cases.append(('update-download-zh-CN', 'chinese', 0, 0,
                  ['--visual', 'mid', '--update-preview', 'download', '--update-percent', '45']))
    records = []
    with tempfile.TemporaryDirectory(prefix='demo_screenshots_') as temp:
        for name, language, page, subpage, scenario in cases:
            bmp = Path(temp) / (name + '.bmp')
            flags = ['--hidden', '--frames', '180', '--set-language', language,
                     '--page', str(page), '--subpage', str(subpage), *scenario]
            command = [str(binary), *flags, '--capture', str(bmp)]
            run = subprocess.run(command, cwd=temp, env={**os.environ, 'SDL_VIDEODRIVER': 'dummy'},
                                 check=True, capture_output=True, text=True, timeout=60)
            report = json.loads(run.stdout)
            code = 'zh-CN' if language == 'chinese' else 'en'
            if (report['result'] != 'PASS' or report['frames'] != 180 or
                    report['page'] != page or report['language'] != code or
                    report['subpage'] != subpage or report['objects'] != report['objects_final']):
                raise RuntimeError('Unexpected simulator report: ' + str(report))
            png = output / (name + '.png')
            with Image.open(bmp) as image:
                rgb = image.convert('RGB')
                if rgb.size != (800, 480) or all(lo == hi for lo, hi in rgb.getextrema()):
                    raise RuntimeError('Wrong dimensions or blank SDL2 frame: ' + name)
                pixels = rgb.tobytes()
                rgb.save(png, optimize=True)
            with Image.open(png) as image:
                if image.convert('RGB').tobytes() != pixels:
                    raise RuntimeError('PNG conversion changed rendered pixels: ' + name)
            records.append({'file': png.name, 'width': 800, 'height': 480, 'sha256': digest(png),
                            'pixel_sha256': hashlib.sha256(pixels).hexdigest(),
                            'arguments': [*flags, '--capture', name + '.bmp'], 'report': report})
    manifest = {'schema': 1, 'captured_at_utc': datetime.now(timezone.utc).isoformat(),
                'renderer': 'LVGL / SDL2 dummy video, RGB pixels preserved from BMP capture',
                'product_base_commit': revision(ROOT), 'product_source_sha256': source_digest(ROOT),
                'framework_base_commit': revision(framework),
                'framework_runtime_sha256': source_digest(framework,
                    ('contracts', 'core', 'runtime', 'diagnostics', 'platform', 'protocols', 'storage', 'ui', 'update')),
                'binary_sha256': digest(binary), 'screenshots': records,
                'scope': 'Host UI evidence only; OTA overlay is a preview, not an installed update.'}
    (output / 'manifest.json').write_text(json.dumps(manifest, ensure_ascii=False, indent=2) + '\n', encoding='utf-8')
    print(json.dumps({'result': 'PASS', 'screenshots': len(records), 'output': str(output)}))


if __name__ == '__main__':
    main()
