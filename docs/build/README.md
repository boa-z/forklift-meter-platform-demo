# Build, run and capture

> [中文版](README.zh-CN.md)

## Build the host Product

Run the following commands in the consuming Framework root, where this repository is checked out as `products/demo`. Install host Python 3, CMake, Ninja, a C/C++ compiler and SDL2 development files first. Keep SDK Python/SCons unchanged.

```text
git submodule update --init --recursive
python -m pip install -r tools/protocol/requirements.txt -r tools/hil/requirements.txt -r tools/ota/requirements.txt
cmake -S . -B build-demo -G Ninja -DCMAKE_BUILD_TYPE=Debug -DMETER_PRODUCT_ROOT=products/demo -DMETER_ENABLE_UPDATE=ON
cmake --build build-demo --parallel
ctest --test-dir build-demo --output-on-failure
```

Pass criteria: configuration selects `products/demo`, build produces `meter-demo`, and CTest reports zero failures. Windows uses `meter-demo.exe`; Linux uses `meter-demo`. If the Product manifest is missing, initialize the submodule. If SDL2 cannot be found, provide `-DSDL2_DIR=PATH_TO_SDL2_CMAKE` and put its runtime DLL directory on PATH.

## Launch and inspect

Windows examples, from the same Framework root:

```text
./build-demo/meter-demo.exe --set-language chinese
./build-demo/meter-demo.exe --hidden --frames 180 --scenario stale --set-language chinese --capture stale.bmp
```

On Linux remove `.exe`. Interactive mode opens an 800×480 window; close it to exit. Automated capture uses a fixed frame count. `--page 0..3` selects dashboard, monitor, faults and settings; `--subpage 1` reaches the second monitor page. Settings uses category buttons and is not a second pager page.

The simulator prints a JSON report. Require `result=PASS`, matching `language`/`page`, the requested completed frame count and stable object count. It generates synthetic input and does not send vehicle CAN.

## Regenerate the actual gallery

Build the current sources first. Use host Python with Pillow available and the SDL2 DLL directory on PATH. This command writes only Product-owned gallery files:

```text
python -m pip install Pillow
python products/demo/tools/capture_screenshots.py --framework-root . --binary build-demo/meter-demo.exe
python tools/check_docs_sync.py --root products/demo
python products/demo/tools/check_docs.py
```

The exporter runs sixteen independent SDL2 processes using the dummy video driver. It converts captured BMP pixels to PNG without resizing or redrawing and verifies dimensions, page, language and simulator status. It stores commands, reports, executable hash, source digest and image hashes in the [manifest](../ui/screenshots/manifest.json). Review every image before committing.

Base commit IDs identify checkout ancestry at capture time; source digests describe the actual captured files, including any uncommitted changes. A source digest mismatch requires rebuilding and exporting again. The manifest is evidence, not a cryptographic firmware signature.

## Firmware and CAN OTA

Select this Product with `METER_PRODUCT_ROOT` in the Framework SDK integration. Follow Framework `docs/build/build.md` and `docs/ota/can-update.md` for SDK image creation, package creation and CAN installation. Board setup and transport are Framework/SDK responsibilities; do not copy a second OTA implementation into this Product.

The gallery's update view is only `--update-preview download`; it neither constructs an OTA package nor transfers or installs firmware. A real OTA result needs the board's before/after identity and captured transport logs.
