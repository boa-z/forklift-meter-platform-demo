#!/usr/bin/env python3
"""从真实 SDL2 帧导出 Product 文档截图，PNG 像素与渲染结果逐字节一致。"""
import argparse
import json
import os
from pathlib import Path
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--binary', type=Path, required=True)
    parser.add_argument('--output', type=Path, default=ROOT / 'docs/ui/screenshots')
    args = parser.parse_args()
    from PIL import Image
    binary, output = args.binary.resolve(), args.output.resolve()
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
    captured = []
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
            captured.append(png.name)
    print(json.dumps({'result': 'PASS', 'screenshots': len(captured), 'output': str(output)}))


if __name__ == '__main__':
    main()
