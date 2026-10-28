#!/usr/bin/env python3
from pathlib import Path
import hashlib
import json
import argparse
import os
ROOT=Path(__file__).resolve().parents[1]
PLATFORM=Path(os.environ.get('METER_PLATFORM_ROOT') or ROOT.parents[1])
p=argparse.ArgumentParser(); p.add_argument('--product-root', type=Path, default=ROOT); ROOT=p.parse_args().product_root.resolve()
m=json.loads((ROOT/'assets/manifest.json').read_text(encoding='utf-8'))
for asset in m['icons']:
    assert all(asset.get(k) for k in ('source','upstream_project','upstream_file','commit','license','modifications','sha256')),asset
    assert asset['license']=='MIT' and len(asset['commit'])==40
    p=ROOT/asset['file']; assert hashlib.sha256(p.read_bytes()).hexdigest()==asset['sha256'],p
    svg=p.read_text(); assert 'viewBox="0 0 24 24"' in svg and 'stroke-width="2"' in svg,p
for license in ('Tabler-MIT.txt','Montserrat-OFL.txt','LVGL-MIT.txt'):
    assert (ROOT/'assets/LICENSES'/license).stat().st_size>500,license
assert (ROOT/'generated/demo_icons.c').is_file()
assert m['font']['license']=='OFL-1.1'
# 内置 Montserrat 字体来自固件链接的同一份 LVGL（SDK 的 lvgl-ui 包），不再有框架自己的 LVGL 版本可核对。
lvgl=Path(os.environ.get('METER_LVGL_ROOT') or PLATFORM.parents[2]/'packages/artinchip/lvgl-ui/lvgl_v9/lvgl')
assert (lvgl/'src/font/lv_font_montserrat_14.c').is_file(),'SDK LVGL font source not found: '+str(lvgl)
print('Asset provenance, dimensions, licenses and hashes PASS')
